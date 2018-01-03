#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<errno.h>
//#include<math.h> //Lier avec -lm
#include<time.h>
#include<dlfcn.h> //Lier avec -ldl

/*Structure de type POSIX ustar*/
struct header_posix_ustar 
{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag[1];
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char pad[12];
};


/*Vérifie la validité du fichier à traiter : nécessairement d'extension 'tar'*/
int fileValidator(char *);
/*Renvoie la taille d'une chaîne de caractères*/
long fileSizeTeller(char *);
/*Renvoie le nombre d'occurences d'un caractère dans une chaîne de caractères*/
int numberCharInString(char * , char);
/*Fonction de détection des paramètres utilisés*/
void usedParameters (int , char **, int *);
/*Fonction de conversion de nombre octal ASCII vers valeur décimale*/
long int asciiOctalToDecimal(char *);
//Conversion de décimal en binaire
int * decimalToBinary (int);
//Correspondance paquet de 03 bits <-> droits sur fichier/répertoire
void numberSplitter(char * , char * );
//Extrait une sous-chaîne à partir d'une chaîne de caratères
char *subString (const char *, unsigned int , unsigned int );
//Vérifie si une chaîne de caractères est faite de caractères ASCII ou non
int isNormalString (char * );
//Renvoie une date formatée AAAA-MM-JJ HH:MM:SS à partir d'un timestamp UNIX
void timestampUNIXToDate (char * , char * );
//Crée les répertoires
void folderCreator(char * , char * );
//Vérifie si un répertoire existe déjà ou pas
int folderExists (char * ); 

//Fonction principale
int main (int argc, char *argv[])
{	
	int i = 0,
	    total_files = 0,
	    total_folders = 0,	
	    * usedParams = malloc(4 * sizeof(int));	
	long number_items_in_archive;
	int fd;
	char * permissions = NULL; 
	char * archiveName = NULL; 
	char * arcN = NULL;
	char * p_conv = malloc(sizeof(char) * strlen("AAAA/MM/JJ HH:MM:SS"));
	permissions = malloc (sizeof(char) * 10);
	//char permissions[10] = {0};
	struct header_posix_ustar * archive_content = NULL;
	struct header_posix_ustar ma_struct;
//Test sur les arguments passés
	usedParameters(argc, argv, usedParams);
	if (argc < 2)
	{
		printf("Nombre d'arguments renseignés insuffisant! Veuillez réassayer.\n");
		return 1;
	}
	if(!fileValidator(argv[argc - 1]))
	{
		printf("Fichier invalide\n");
		return 1;
	}
	archiveName = malloc(sizeof(char) * strlen(argv[argc - 1]));
	strcpy (archiveName, argv[argc - 1]);
//	printf("Nom du dossier de l'archive : %s \n", archiveName);
	strcat(argv[argc - 1],".tar");

//Etape 1
	if ((fd = open (argv[argc - 1], O_RDONLY, 0)) == -1)
	{
		printf("Erreur survenue lors de l'ouverture de l'archive : %s\n", argv[argc - 1]);
		return 1;
	}
//Retrouver le nombre d'éléments dans l'archive
	number_items_in_archive = fileSizeTeller(argv[argc - 1]) / 512;

//Allocation de la place pour les éléments de l'archive dans un tableau dynamique
	archive_content = malloc(number_items_in_archive * 512);
	int o = 0;
	while(read(fd, &ma_struct, 512) > 0)
	{
		archive_content[i++] = ma_struct;
		
		if (isNormalString(ma_struct.name) == 1 && (strlen(ma_struct.name) != 1	 && (int)ma_struct.name[0] != 32))
		{
			arcN = malloc(sizeof(char) * (strlen(archiveName) + strlen(ma_struct.name) + 1));
			strcpy (arcN, archiveName);
			arcN = strcat(strcat(arcN, "/"), ma_struct.name);
			timestampUNIXToDate(ma_struct.mtime, p_conv);
			//archiveName = (strcat(archiveName, "/"));
			if(strcmp(ma_struct.typeflag,"0") == 0 && strlen(ma_struct.typeflag) == 1 && strlen(ma_struct.name) != 0)
			{
				if (permissions != NULL)
					numberSplitter(ma_struct.mode, permissions);
				if (usedParams[0] == 1)
					printf("-%s %ld/%ld %ld %s ", 	
						permissions,
						asciiOctalToDecimal(&ma_struct.uid[0]), 
						asciiOctalToDecimal(&ma_struct.gid[0]),
						asciiOctalToDecimal(&ma_struct.size[0]), 
						p_conv);
				printf("%s \n", ma_struct.name);				
				/*printf("!Oct%s \n", ma_struct.mode);
				printf("!Dec%ld \n", asciiOctalToDecimal(ma_struct.mode));*/
				total_files += 1;
			}

			if(strcmp(ma_struct.typeflag,"5") == 0 && strlen(ma_struct.typeflag) == 1 && strlen(ma_struct.name) != 0)
			{			
				if (permissions != NULL)
					numberSplitter(ma_struct.mode, permissions);
				if (usedParams[0] == 1)
					printf("d%s %ld/%ld %ld %s ",
						permissions,
						asciiOctalToDecimal(&ma_struct.uid[0]), 
						asciiOctalToDecimal(&ma_struct.gid[0]),
						asciiOctalToDecimal(&ma_struct.size[0]), 
						p_conv);
				printf("%s \n", ma_struct.name);				
				if (usedParams[2] == 1)
				{
					folderCreator(arcN, ma_struct.mode);
						
				}
				total_folders += 1;
			}
		
		
			if (ma_struct.typeflag[0] == '2' && ma_struct.typeflag[1] == '/' && strlen(ma_struct.name) != 0)
			{
				if (permissions != NULL)
					numberSplitter(ma_struct.mode, permissions);
				if (usedParams[0] == 1)
					printf("l%s %ld/%ld %ld %s ",  
						permissions,
						asciiOctalToDecimal(&ma_struct.uid[0]), 
						asciiOctalToDecimal(&ma_struct.gid[0]),
						asciiOctalToDecimal(&ma_struct.size[0]), 
						p_conv);
				printf("%s -> %s\n", 
					ma_struct.name,
					subString(ma_struct.typeflag, 1, strlen(ma_struct.typeflag)));
				total_files += 1;
			}	
		}
			
	}
	free (archiveName);
	free (arcN);
	free (usedParams);
	close(fd);
	return 0;
}

int fileValidator(char* stringToParse)
{
	char * pointeur = NULL;
	pointeur = strtok (stringToParse, ".");
	int tar = 0,
	    nb_mot = 1,
	    gz = 0;

	while(pointeur != NULL)
	{
		pointeur = strtok(NULL, ".");
		if (pointeur != NULL )
		{
		    if (strcmp(pointeur, "tar") == 0 && nb_mot == 1)
			tar = 1;
		    if (strcmp(pointeur, "gz") == 0 && nb_mot == 2)
			gz = 1;
		    nb_mot++;
		}
	}
	if ((nb_mot == 3 && tar == 1 && gz == 1) || (nb_mot == 2 && tar == 1 && gz == 0))
	{
		return 1;
	}
	else
		return 0;
}

long fileSizeTeller(char * fileName)
{
	FILE * f; 
   	long ret = 0; 
  
    	f = fopen(fileName, "rb");    
    	if (f != NULL) 
    	{ 
		fseek(f, 0, SEEK_END);
		ret = ftell(f);
		fclose(f); 
    	}
	else
		perror(NULL);

    	return ret; 
}

int numberCharInString(char * stringWhereToSeekCharacter, char character_to_seek)
{
	int i,
            result = 0,
            charSize = strlen(stringWhereToSeekCharacter);
    for (i = 0; i < charSize; i++)
    {
        if (stringWhereToSeekCharacter[i] == character_to_seek)
            result += 1;
    }
	return result;
}

void usedParameters (int argc, char * argv[], int * usedParams)
{
	char optstring[] = "lpxz";
	int c, 
	    i,
	    authorizedNumberOfParams;
///*	authorizedNumberOfParams = strlen(optstring);
//	usedParams = malloc(authorizedNumberOfParams * sizeof(int)); */
	
	for (i = 0; i < authorizedNumberOfParams; i++)
	{
		usedParams[i] = 0;
	}
	while((c = getopt (argc, argv, optstring)) != EOF)
	{
		switch(c)	
		{
			case 'l':
				usedParams[0] = 1;
			break;

			case 'p':
				usedParams[1] = 1;
			break;

			case 'x':
				usedParams[2] = 1;
			break;

			case 'z':
				usedParams[3] = 1;
			break;
		}
	}
}

long int asciiOctalToDecimal(char * valueToConvert)
{
	long int decimal = 0;
	char *p_conv;
	decimal = (long)strtol (valueToConvert, &p_conv, 8);
	return decimal;
}

int * decimalToBinary (int decimal)
{
	int m,
	    i = 0,
	    c = 0,
	    * binary = NULL;
	binary = malloc(sizeof(int) * 3);
	if (binary == NULL)
		exit(1);
	while(decimal >= 1)
	{
		m = decimal % 2;
		binary[i] = m;
		i = i + 1;
		c = c + 1;
		decimal = decimal / 2;
	}
	return binary;
}

void numberSplitter(char * valueToParse, char * result)
{
	int * tab = NULL,
	    i = 0,
	    j = 0,
	    number = 0,
	    octalValue;

	char nothing[4] = {'-', '-', '-'},
	     owner[4] = {'-', '-', '-'},
	     group[4] = {'-', '-', '-'},
	     other[4] = {'-', '-', '-'};
	char * parsedString = NULL;
	parsedString = malloc (sizeof(char) * 10);
	if (valueToParse != NULL)
		octalValue = atoi(valueToParse);

	while (octalValue != 0)
	{
		number = octalValue % 10;
		tab = decimalToBinary(number);
		if (tab[2] == 1)
			nothing[0] = 'r';

		if (tab[1] == 1)
			nothing[1] = 'w';

		if (tab[0] == 1)
			nothing[2] = 'x';

		if (i == 0)
			strcpy(other, nothing);

		if (i == 1)
			strcpy(group, nothing);

		if (i == 2)
			strcpy(owner, nothing);

		octalValue /= 10;
		i += 1;
		nothing[0] = '-';
		nothing[1] = '-';
		nothing[2] = '-';
	}
	parsedString = strcat(owner, strcat(group, other));
	parsedString[9] = '\0';
	strcpy(result, parsedString);
}

char * subString (const char *s, unsigned int start, unsigned int end)
{
   char *new_s = NULL;

   if (s != NULL && start < end)
   {
      new_s = malloc (sizeof (*new_s) * (end - start + 2));
      if (new_s != NULL)
      {
         int i;

         for (i = start; i <= end; i++)
         {
            new_s[i-start] = s[i];
         }
         new_s[i-start] = '\0';
      }
      
      else
      {
         fprintf (stderr, "Memoire insuffisante\n");
         exit (EXIT_FAILURE);
      }
   }
   return new_s;
}

int isNormalString (char * stringToParse)
{
	int o, s, result = 1, los = strlen(stringToParse);
	for (o = 0; o < los; o++)
	{
		s = stringToParse[o];
		if (s < 0 || s > 255)
			result = 0;
	}
	return result;
}

void timestampUNIXToDate (char * timestamp, char * formattedDate)
{
    char * p_conv;
    long a = (long)strtol (timestamp, &p_conv, 8);
    time_t maDate = a;
    struct tm tm_maDate = * localtime (&maDate);
    char s_maDate[sizeof "AAAA/MM/JJ HH:MM:SS"];
    strftime (s_maDate, sizeof s_maDate, "%Y-%m-%d %H:%M:%S", &tm_maDate);
    strcpy(formattedDate, s_maDate);
}

int folderExists (char * path)
{	
	int result = 1;
	FILE * folder = NULL;
	folder = fopen(path,  "r");
	
	if (folder == NULL)
  		result = 0;
  	//fclose(folder);
	return result;	
}

void folderCreator(char * path, char * mode)
{
	printf("Nom du dossier de l'archive : %s \n", path);
	
		char * pattern = NULL;
		char * token, * token2;
		pattern = malloc(sizeof(char) * strlen(path));		
		token = strtok(path, "/");
		while(token != NULL)
		{
			token2 = malloc(sizeof(char) * strlen(token));
			strcpy(token2, token);
			strcat(token2, "/");
			strcat(pattern , token2);
			if (folderExists(pattern) == 0)
			{
				if(!mkdir(pattern, asciiOctalToDecimal(mode)))
					printf("Problème survenu lors de la création du dossier %s\n", pattern);
			}
			token = strtok(NULL, "/");
	}
}

