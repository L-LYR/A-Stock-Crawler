# Stock Crawler

> This is a stock crawler programed in C.
> Just for studying simple concurrent programming and socket programing.

## How to use

You need to get the current IP of hq.sinajs.cn first.

And than create a directory named `data` to hold the results.

Last, compile and run `crawler` by the commands bellow.

``` shell
gcc -lpthread -o crawler crawler.c

./crawler
```

