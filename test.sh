./build.sh
rm -f out.txt
./swizzle examples/argv.swz test > out.txt
./swizzle examples/ascii.swz >> out.txt
./swizzle examples/bf.swz >> out.txt
./swizzle examples/echo.swz >> out.txt
./swizzle examples/fizzbuzz.swz >> out.txt
./swizzle examples/hello.swz >> out.txt
./swizzle examples/loop_test.swz >> out.txt
./swizzle examples/ls_test.swz >> out.txt
./swizzle examples/octal.swz >> out.txt
./swizzle examples/quine.swz >> out.txt
./swizzle examples/string.swz >> out.txt
./swizzle examples/swz.swz examples/hello.swz >> out.txt
./swizzle examples/swz.swz examples/swz.swz examples/hello.swz >> out.txt
diff test.txt out.txt
