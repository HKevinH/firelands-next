// Collision-pipeline tasks for firelands-extractors TUI only (not linked into
// FirelandsExtractCommon — subprocess paths are set by CMake).

#include "ExtractorTasks.h"

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <ostream>
#include <sstream>
#include <string>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace firelands::extract {
namespace {

static std::string ShellQuoteArg(std::string const& s) {
    if (s.find_first_of(" \t\n\r\"'$`") == std::string::npos) {
        return s;
    }
    std::string r = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') {
            r.push_back('\\');
        }
        r.push_back(c);
    }
    r.push_back('"');
    return r;
}

#ifndef _WIN32
static int InterpretWaitStatus(int st) {
    if (WIFEXITED(st)) {
        return WEXITSTATUS(st);
    }
    return -1;
}
#endif

/// Split incoming bytes on newlines and write complete lines to `out`.
static void ConsumeRawBytes(std::string& carry, char const* data, std::size_t n,
                            std::ostream& out) {
    carry.append(data, n);
    for (;;) {
        std::size_t const pos = carry.find('\n');
        if (pos == std::string::npos) {
            break;
        }
        std::string line = carry.substr(0, pos);
        while (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        out << line << '\n';
        carry.erase(0, pos + 1);
    }
}

static void FlushCarryLine(std::string& carry, std::ostream& out) {
    if (carry.empty()) {
        return;
    }
    while (!carry.empty() && carry.back() == '\r') {
        carry.pop_back();
    }
    out << carry << '\n';
    carry.clear();
}

#if defined(_WIN32)

static int RunProcessCapture(std::string const& command, std::ostream& out, std::ostream& err) {
    out << "[spawn] " << command << "\n";
    out.flush();

    // _popen only reads stdout; merge stderr like the POSIX pipe setup.
    std::string const cmd_line = command + " 2>&1";
    FILE* pipe = _popen(cmd_line.c_str(), "r");
    if (pipe == nullptr) {
        err << "[error] _popen failed (errno " << errno << ")\n";
        return -1;
    }

    std::string carry;
    std::array<char, 8192> buf{};
    for (;;) {
        std::size_t const n = std::fread(buf.data(), 1, buf.size(), pipe);
        if (n > 0) {
            ConsumeRawBytes(carry, buf.data(), n, out);
            out.flush();
        } else {
            if (std::ferror(pipe) != 0) {
                err << "[error] read from subprocess pipe failed\n";
            }
            break;
        }
    }
    FlushCarryLine(carry, out);
    out.flush();

    int const st = _pclose(pipe);
    // MSVC encodes process exit as high byte; other hosts may return the code directly.
    int const code =
        (st == -1) ? -1
                   : (static_cast<unsigned>(st) > 255U ? ((st >> 8) & 0xFF) : (st & 0xFF));
    if (code != 0) {
        err << "[error] subprocess exited with code " << code << "\n";
    }
    return code;
}

#else

static int RunProcessCapture(std::string const& command, std::ostream& out, std::ostream& err) {
    out << "[spawn] " << command << "\n";
    out.flush();

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        err << "[error] pipe() failed (errno " << errno << ")\n";
        return -1;
    }

    pid_t const pid = fork();
    if (pid == -1) {
        err << "[error] fork() failed (errno " << errno << ")\n";
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0 || dup2(pipefd[1], STDERR_FILENO) < 0) {
            _exit(126);
        }
        close(pipefd[1]);
        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipefd[1]);

    std::string carry;
    std::array<char, 8192> buf{};
    for (;;) {
        ssize_t const n = read(pipefd[0], buf.data(), buf.size());
        if (n > 0) {
            ConsumeRawBytes(carry, buf.data(), static_cast<std::size_t>(n), out);
            out.flush();
        } else if (n == 0) {
            break;
        } else {
            if (errno == EINTR) {
                continue;
            }
            err << "[error] read(pipe) failed (errno " << errno << ")\n";
            break;
        }
    }
    FlushCarryLine(carry, out);
    out.flush();

    close(pipefd[0]);

    int st = 0;
    if (waitpid(pid, &st, 0) < 0) {
        err << "[error] waitpid failed (errno " << errno << ")\n";
        return -1;
    }

    int const code = InterpretWaitStatus(st);
    if (code != 0) {
        err << "[error] subprocess exited with code " << code << "\n";
    }
    return code;
}

#endif

} // namespace

int RunServerMapVmapExtractTask(const std::filesystem::path& dataDir,
                                const std::filesystem::path& outDir, std::ostream& out,
                                std::ostream& err) {
    if (!std::filesystem::is_directory(dataDir)) {
        err << "Data directory missing or not a directory: " << dataDir.string() << "\n";
        return 1;
    }
#ifndef FIRELANDS_MAP_EXTRACTOR_PATH
#error "FIRELANDS_MAP_EXTRACTOR_PATH must be set by CMake for firelands-extractors"
#endif
    // firelands-map-extractor-vmap: -d is WoW install root (tool appends /Data).
    std::filesystem::path installRoot = dataDir.parent_path();
    if (installRoot.empty()) {
        installRoot = std::filesystem::path(".");
    }
    std::ostringstream cmd;
    cmd << ShellQuoteArg(FIRELANDS_MAP_EXTRACTOR_PATH) << " -d "
        << ShellQuoteArg(installRoot.string()) << " -o " << ShellQuoteArg(outDir.string());
    return RunProcessCapture(cmd.str(), out, err) == 0 ? 0 : 1;
}

int RunVmap4ExtractorSubprocess(const std::filesystem::path& wowDataDir,
                                const std::filesystem::path& collisionOutRoot, std::ostream& out,
                                std::ostream& err) {
    if (!std::filesystem::is_directory(wowDataDir)) {
        err << "Data directory missing: " << wowDataDir.string() << "\n";
        return 1;
    }
    std::filesystem::path const wowInstall = wowDataDir.parent_path();
#ifndef FIRELANDS_VMAP4_EXTRACTOR_PATH
#error "FIRELANDS_VMAP4_EXTRACTOR_PATH must be set by CMake for firelands-extractors"
#endif
    std::ostringstream cmd;
    cmd << ShellQuoteArg(FIRELANDS_VMAP4_EXTRACTOR_PATH) << " -d "
        << ShellQuoteArg(wowInstall.string()) << " -o " << ShellQuoteArg(collisionOutRoot.string())
        << " -q";
    return RunProcessCapture(cmd.str(), out, err) == 0 ? 0 : 1;
}

int RunVmap4AssemblerSubprocess(const std::filesystem::path& collisionOutRoot, std::ostream& out,
                                std::ostream& err) {
    std::filesystem::path const buildings = collisionOutRoot / "Buildings";
    std::filesystem::path const vmaps = collisionOutRoot / "vmaps";
    if (!std::filesystem::is_directory(buildings)) {
        err << "Buildings/ not found under " << collisionOutRoot.string()
            << " — run VMAP4 extract first.\n";
        return 1;
    }
#ifndef FIRELANDS_VMAP4_ASSEMBLER_PATH
#error "FIRELANDS_VMAP4_ASSEMBLER_PATH must be set by CMake for firelands-extractors"
#endif
    std::ostringstream cmd;
    cmd << ShellQuoteArg(FIRELANDS_VMAP4_ASSEMBLER_PATH) << " "
        << ShellQuoteArg(buildings.string()) << " " << ShellQuoteArg(vmaps.string());
    return RunProcessCapture(cmd.str(), out, err) == 0 ? 0 : 1;
}

} // namespace firelands::extract
