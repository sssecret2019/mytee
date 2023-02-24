#include <stdlib.h>
#include <stdio.h>

int main(){
	pid_t pid = fork();
	char *argv[] = {"bash", "-c", "XAUTHORITY=$(ls -1 /home/*/.Xauthority | head -n 1 ) xset led named \"Scroll Lock\"", 0};
	char *env[] = {"DISPLAY=:0", NULL};
	switch(pid)
	{
		case 0:	

			execve("/bin/bash", argv, env);
		default:
			wait((int*)0);
	}
			
	char *argv2[] = {"bash", "-c", "XAUTHORITY=$(ls -1 /home/*/.Xauthority | head -n 1 ) xset -led named \"Scroll Lock\"", 0};
	char *env2[] = {"DISPLAY=:0", NULL};
	pid = fork();
	switch(pid)
	{
		case 0:	
			execve("/bin/bash", argv2, env2);
		default:
			wait((int*)0);
	}

	return 0;
}
