# FFC 

fast(relatively) file copy program


### Compile
```
    clang++ -std=c++20 -O3 -march=native -flto -fuse-ld=lld -Wall -Wextra -DNDEBUG -o file_copier.exe src/main.cc src/Logger.cc src/DirectoryCopier.cc src/Utils.cc
```
### Compiler Flags

```
    -std=c++20 : C++20 standart
    -03: Most aggresive optimization for speed 
    -march=native: optimization for architecture of user's cpu
    -flto: Link-Time optimization
    -fuse-ld=lld: for flto
    -Wall -Wextra: Code Quailty
    -DNDEBUG: disables standart asserts
```

### Usage
```
    ./file_copier.exe "Source Path" "Destination Path"
```