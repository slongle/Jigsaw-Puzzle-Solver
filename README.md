64# Jigsaw Puzzle Solver
## Introduction
This project is for [Huawei Honorcup Marathon 2](http://codeforces.com/blog/entry/70047).  
There are a set of images , each has a size of 512×512 pixels. Divide each image into $m×m$ square fragments of $p×p(p=64/32/16)$ pixels $(m=512/p)$ and rearrange them to restore the original image.  
## Result
![](https://postimg.cc/PLB2RWTN)  
64x64  
![](https://postimg.cc/p99sCGNg)  
32x32  
![](https://postimg.cc/68bHdsPr)  
16x16
## Compile
This project are all .h files except main.cpp, so you can build it easily like this  
```bash
g++ -O2 -o jigsaw src/main.cpp -std=c++17   
```
## Reference
[An Automatic Solver for Very Large Jigsaw Puzzles
Using Genetic Algorithms](https://arxiv.org/pdf/1711.06767.pdf)