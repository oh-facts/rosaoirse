## Generic Title


Under construction.

<!--

I render to a pixel buffer and then blit it to the window's surface. I use miniaudio for audio and stb_ttf to parse and rasterize ttfs. Both are inside `src/lib`.

## compile

cmake file is very simple. Literally just do `cmake ..` inside a build directory, no extra flags needed.
```
//Inside the root directory
mdkir build
cd build
cmake ..
```

I personally use ninja + clang, so I do this
```
mdkir build
cd build
cmake .. -DCMAKE_C_COMPILER=clang -G "Ninja"
ninja
```

Good job! You are so cool! Now, download this [drive link](https://drive.google.com/drive/folders/1M4K7Ur9gShpLSmHQQxYbpiguzK3a7oSH?usp=sharing). It contains the resource folder. Unzip it and place it in the root directory. So,

```
root/
├─ src/
├─ res/
│  ├─ *.wav
│  ├─ *.bmp
│  ...
│
├─ README.md
...
```
Now you should be able to run the binary. (If you don't put the resources, it will segfault because of assert that requires that you have the resources)

## contributing

Open an issue before you decide to work on it. I am unsure what one would want to add to this, since even I haven't been very clear about this engine's purpose, but please feel free to do whatever. Doing audio properly comes to mind. I am probably working on them already so open an issue first.

If you own a linux or a mac, feel free to work on the respective platform layers. I am not using sdl anymore. Check `docs/platform_layer` to know what all needs to be implemented. Look at the win32 platform layer for more reference. I am probably already working on the linux one. I don't own a mac so there won't be mac compatibility until then. Sorry.

<!-- //ctrl shift v (my vscode md viewer plugin keybind) -->