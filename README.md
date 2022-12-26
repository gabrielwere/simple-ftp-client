## SIMPLE PYFTPDLIB CLIENT

A simple ftp client for pyftpdlib server.<br>
The program was tested with pyftpdlib 1.5.4.<br>

#### USAGE

You need to have python and pyftpdlib installed first.<br> 
On a linux machine,start pyftpdlib.

```
python3 -m pyftpdlib -p 21

```

Depending on which Linux distro you use,the command could be slightly different.You may need root priveleges(sudo),you may not need to specify which python version you are using,among other things.<br>

On another terminal,run the commands

```
gcc client.c -o client
./client your_ip_address

```
The files and folders available will then be listed.<br>


