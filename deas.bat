@echo off
flpcipd BNC
e:
cd \deasbin
deasbin.exe hanoi
flpcipd -u
echo.
