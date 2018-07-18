# swizzle
An esoteric programming language putty

Quick Examples:
- Hello World: ```10 "Hello, World!%c" P;;```
- FizzBuzz: 
```
{M
    1=i
    (i 100 <=?
        (i 15 % 0 ==?
            "FizzBuzz" P;
        :(i 3 % 0 ==?
            "Fizz" P;
        :(i 5 % 0 ==?
            "Buzz" P;
        :
            i "%d" P;;
        )))
        10 "%c" P;;
        i++;
    @)
}
M
```

Swizzle is an interpreted language built around a very simple core loop
that does two things, look up functions and execute them. It doesn't 
need much to bootstrap itself into a useful substrate, and *almost* everything
is completely modular and optional.

Swizzle supports:
- inline strings using quotes: ```"..."```
- numbers, both single digit, and multi-digit: ```1```, ```9001```
- single-letter function definitions: ```{F <function contents> }``` (in this case, F is the function's name)
- single-letter variables: ```i```
- printf, dlsym, and exit: ```P```, ```D```, ```E```
- basic math: ```+```, ```-```, ```*```, ```/```, ```%```
- conditionals and loops: ```(1 1 == ? <true> : <false> @)```   
```@``` indicates a loop back to the nearest same-level ```(```, ```?``` is just a standard  
c-like ternary operator, branching based on the value on the top of the stack
