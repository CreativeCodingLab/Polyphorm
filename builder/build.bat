if not exist "bin" mkdir bin
if not exist "bin\\obj" mkdir bin\\obj
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
call cl.exe main.cpp /nologo /W3 /Od /RTC1 /MTd /EHsc /Zc:inline /Gm- /MP /Fo:bin\\obj\\ /Fd:bin\\ /Zi /DDEBUG /link /INCREMENTAL:NO /subsystem:console /MACHINE:X64 /debug:fastlink /CGTHREADS:4 Shlwapi.lib Advapi32.lib ole32.lib oleaut32.lib /OUT:bin\\build.exe