First, build libmatoya

cl /I..\src 0-minimal.c matoya.lib bcrypt.lib d3d11.lib d3d9.lib hid.lib uuid.lib dxguid.lib opengl32.lib ws2_32.lib user32.lib gdi32.lib xinput.lib ole32.lib shell32.lib windowscodecs.lib shlwapi.lib imm32.lib winmm.lib /link /libpath:..\bin\windows\x64 