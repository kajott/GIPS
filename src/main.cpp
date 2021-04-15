#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <cstdlib>
    #include <cstring>
    #include "string_util.h"
#endif

#include "gips_app.h"

int main(int argc, char* argv[]) {
    static GIPS::App app;
    return app.run(argc, argv);
}

#ifdef _WIN32
    int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
        (void)hInstance, (void)hPrevInstance, (void)lpCmdLine, (void)nCmdShow;

        char* cmdLine = StringUtil::copy(GetCommandLineA());
        int argc = 0;
        char** argv = new(std::nothrow) char*[strlen(cmdLine)];
        if (!argv) return 99;

        char *pSrc = cmdLine;
        char *pDest = cmdLine;
        bool inParam = false;
        bool inQuote = false;
        for (;  *pSrc;  ++pSrc, ++pDest) {
            *pDest = *pSrc;
            switch (*pSrc) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if (inParam && !inQuote) {
                        *pDest = '\0';
                        inParam = false;
                    }
                    break;
                case '"':
                    if (inParam)
                        inQuote = !inQuote;
                    else {
                        argv[argc++] = pDest;
                        inParam = inQuote = true;
                    }
                    --pDest;
                    break;
                case '\\':
                    if (inParam && inQuote && pSrc[1] == '"') {
                        *pDest = '"';
                        ++pSrc;
                    }
                default:
                    if (!inParam) {
                        argv[argc++] = pDest;
                        inParam = true;
                        inQuote = false;
                    }
                    break;
            }
        }
        *pDest = '\0';
        int ret = main(argc, argv);
        delete[] argv;
        ::free(cmdLine);
        return ret;
    }
#endif
