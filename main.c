#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


void ChangeDirectory(char* path,char* actualDirect);
void ExecCommand(char* commande);
void ParseCommand(char* commande,char** commandeAExec, bool* redirectionS, bool* redirectionErr, bool* redirEffa, bool* input, char* nomFichier, char* nomFichierErr, char* nomInput);

int main(void){

	char* commande;
	char** arg;
	int nbArg = 0;
	char* directory;
	char* comCopy;
	comCopy = malloc(sizeof(char)*4096);
	commande = malloc(sizeof(char)*4096);
	arg = malloc(sizeof(char)*1024);
	directory = malloc(sizeof(char)*1024);
	

	getcwd(directory,1024);

	printf("\e[1;1H\e[2J"); //Permet de nettoyer l'ecran

	printf("\033[0;33m"); //Changement de couleur pour le shell
	printf("%s%% ",directory);
	printf("\033[0m");
		

	while(1){
	
		if(fgets(commande,4096,stdin) == NULL)
			break;	
		strcpy(comCopy,commande);
		strtok(comCopy,"\n");
		// decomposition de la commande dans un tableau d'argument
		char* pt = commande;
		while((arg[nbArg] = strsep(&pt," ")) != NULL){
			printf("%s \n", arg[nbArg]);
			nbArg ++;
		}


		//Enleve le  retour a la ligne du dernier argument
		strtok(arg[nbArg-1],"\n");
		
	
		// Debut de gestion de la commande
		if(strcmp("exit",arg[0]) == 0){
			break;
		}
		else if(strcmp("cd",arg[0]) == 0){
			ChangeDirectory(arg[1],directory);	
		}else{
			if(strcmp("&",arg[nbArg-1])==0){
				if(fork() == 0){
					ExecCommand(comCopy);
				}
			}else
				ExecCommand(comCopy);
		}

		//Phase de reinitialisation

		printf("\033[0;33m");
		printf("%s%% ",directory);
		printf("\033[0m");
		nbArg = 0;
	}
	printf("\n");


	free(commande);
	free(arg);
	free(directory);
	free(comCopy);
	
}




void ChangeDirectory(char* path, char* actualDirect){
	if(chdir(path) < 0){
		printf("Chemin inconnu impossible de changer de repertoire\n");
	}else{
		getcwd(actualDirect,1024);
	}

}


void ExecCommand(char* commande){
	char ** argument;
	argument = malloc(sizeof(char)*256);


	int fileRedi;
	int fileErr;
	int fileIn;
	char* fileNameRedi;
	char* fileNameErr;
	char* fileNameIn;

	fileNameRedi = malloc(sizeof(char)*64);
	fileNameErr = malloc(sizeof(char)*64);
	fileNameIn = malloc(sizeof(char)*64);

	bool redirectionS = false;
	bool redirEffa = false;
	bool redirectionErr = false;
	bool inputFile = false;


	//Traitement du pipe
	
	bool tuyeau = false;

	int nbPipe = 0;
	char** listeCommandePipe;
	listeCommandePipe = malloc(sizeof(char)*512);

	//Decomposition de la commande en fonction des tuyeau
	char* pt = commande;

	while((listeCommandePipe[nbPipe] = strsep(&pt,"|")) != NULL){
		nbPipe ++;
		if(nbPipe > 1)
			tuyeau = true;
	}

	int tube[2];
	int entree = 0;


	pid_t pid;
	for(int i=0; i<nbPipe; i++){
		pipe(tube);
		pid = fork();


		if(pid<0){
			printf("Erreur crea fils");
		}else if(pid == 0){

			if( i == nbPipe-1){
				tuyeau = false;
			}


			ParseCommand(listeCommandePipe[i], argument, &redirectionS, &redirectionErr, &redirEffa, &inputFile, fileNameRedi, fileNameErr, fileNameIn);

			if(redirectionS && !tuyeau){
				fileRedi = open(fileNameRedi, O_WRONLY | O_APPEND | O_CREAT, 0666);
				dup2(fileRedi,1);
				close(fileRedi);
			}
			if(redirectionErr){
				fileErr = open(fileNameErr, O_WRONLY | O_APPEND | O_CREAT, 0666);
				dup2(fileErr,2);
				close(fileErr);
			}
			if(redirEffa && !tuyeau){

				int del = remove(fileNameRedi);

				if(del==0){
					fileRedi = open(fileNameRedi, O_WRONLY | O_APPEND | O_CREAT, 0666);
					dup2(fileRedi,1);
					close(fileRedi);
				}else{
					printf("Pas de fichier a effacer creation du fichier\n");
					fileRedi = open(fileNameRedi, O_WRONLY | O_APPEND | O_CREAT, 0666);
					dup2(fileRedi,1);
					close(fileRedi);
				}

			}
			if(inputFile){
				fileIn = open(fileNameIn, O_RDONLY);
				if(fileIn < 0)
					printf("Fichier inexistant");
				else{
					dup2(fileIn,0);
					close(fileIn);
				}


			}

			//Debut de la partie de gestion du pipe
			//Tant que on est pas a la derniere iteration on redirige
			if(tuyeau){
				dup2(tube[1],1); //Lui a enlever sur la derniere
				dup2(entree,0);
				close(tube[0]);
				close(tube[1]);
			}else{ //traitement dernier pipe ou on doit sortir
				dup2(entree,0);
				close(tube[0]);
				close(tube[1]);
			}

			if(execvp(argument[0],argument) < 0){
				printf("Commande inconnu\n");
			}
		
		}else{ //Pere
			close(tube[1]);
			entree = tube[0];
			wait(NULL);
		}

	}
	free(argument);
	free(fileNameRedi);
	free(fileNameErr);
	free(fileNameIn);
}


//Fonction permettant de detecter les parametre de la commande ainsi que ses redirections si il y en as
void ParseCommand(char* commande,char** commandeAExec, bool* redirectionS, bool* redirectionErr, bool* redirEffa, bool* input, char* nomFichier, char* nomFichierErr, char* nomInput){
	char* tmp;
	tmp = malloc(sizeof(char)*126);
	int nbParam = 0;
	char* pt = commande;
	while((tmp = strsep(&pt," ")) != NULL){
		if(strcmp(tmp,">") == 0){
			*redirectionS = true;
			char* nF = strsep(&pt," ");
			strcpy(nomFichier,nF);
			strtok(nomFichier,"\n");
		}
		else if(strcmp(tmp,">>") == 0){
			*redirEffa = true;
			char* nF = strsep(&pt," ");
			strcpy(nomFichier,nF);
			strtok(nomFichier,"\n");
		}
		else if(strcmp(tmp,"2>") == 0){
			*redirectionErr = true;
			char* nFE = strsep(&pt," "); 
			strcpy(nomFichierErr,nFE);
			strtok(nomFichierErr,"\n");
		}
		else if(strcmp(tmp,"<") == 0){
			*input = true;
			char* nIn = strsep(&pt," ");
			strcpy(nomInput,nIn);
			strtok(nomInput,"\n");
		}
		else if(strcmp(tmp,"") == 0){
		}
		else if(strcmp(tmp,"&") == 0){
		}
		else{
			commandeAExec[nbParam] = tmp;
			nbParam ++;
		}
	}

	strtok(commandeAExec[nbParam-1],"\n");
	
	free(tmp);


}
