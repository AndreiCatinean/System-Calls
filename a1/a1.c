#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct section_head
{
	char name[13];
	char type;
	int offset;
	int size;
}section;

//optiunea extract dezactiveaza print-urile si valideaza o anumita sectiune, iar findall valideaza fisierul si faciliteaza returnarea nr-ului de sectiuni
int parse(const char* path, bool extract, section** head_extr,int section_validate,bool findall)
{
	int file = open(path, O_RDONLY);
	if(file == -1)
	{	
		perror("Could not open input file");
        return -1;
	}

	char magic[5];
	read(file,magic,4);
	magic[4]='\0';

	if(strcmp("OQPZ",magic)!=0)
	{
		if(extract==false)
		{
			printf("ERROR\n");
		printf("wrong magic\n");
		}
		
		close(file);
		return -1;
	}

	lseek(file,2,SEEK_CUR);

	
	int version = 0;
	read(file,&version,4);

	if(version<69 || version>108)
	{
		if(extract==false)
		{
			printf("ERROR\n");
			printf("wrong version\n");
		}
		
		close(file);
		return -1;
	}

	char sect_nr;
	read(file,&sect_nr,1);
	
	if( (int)sect_nr<6 || (int)sect_nr>10)
	{
		if(extract==false)
		{
			printf("ERROR\n");
			printf("wrong sect_nr\n");
		}
		
		close(file);
		return -1;
	}


	section* headers=(section*)malloc((int)sect_nr*sizeof(section));

	for(int i=0;i<sect_nr;i++)  //citirea struct-urilor camp cu camp
	{
		read(file,headers[i].name,12);
		headers[i].name[12]='\0';
		read(file,&headers[i].type,1);

		if(!((int)headers[i].type==16 || (int)headers[i].type==91 || 
			(int)headers[i].type==37 || (int)headers[i].type==97||
			(int)headers[i].type==79 || (int)headers[i].type==67))
		{
			if(extract==false)
			{
				printf("ERROR\n");
				printf("wrong sect_types\n");
			}
			
			close(file);
			free(headers);
			return -1;
		}

		read(file,&headers[i].offset,4);
		read(file,&headers[i].size,4);

	}

	if(extract==false)
	{
		printf("SUCCESS\n");
		printf("version=%d\n",version);
		printf("nr_sections=%d\n",(int)sect_nr);
		for(int i=0;i<sect_nr;i++)
		{
			printf("section%d: %s %d %d\n",i+1,headers[i].name,(int)headers[i].type,headers[i].size);

		}
		free(headers);
	}
	else
		*head_extr=headers;  
	

	if(extract==true && (int)sect_nr<section_validate && findall==false)
		{
			close(file);
			return -2;
		}
	
	close(file);

	return (int)sect_nr;

}

int extract_section(const char* path, section* headers, int section_number, int line,bool findall)
{
	int file = open(path, O_RDONLY);
	if(file == -1)
	{
		perror("Could not open input file");
        return 1;
	}
	
	lseek(file,headers[section_number-1].offset,SEEK_SET); //mutam cursorul la offset-ul sectiunii
	char* section_buffer=(char*)malloc(headers[section_number-1].size*sizeof(char));
	read(file,section_buffer,headers[section_number-1].size);  //citim toata sectiunea

	int line_nr=1;
	for(int i=0;i<headers[section_number-1].size-1;i++)
		if ((int)section_buffer[i] == 0x0D && (int)section_buffer[i+1] == 0x0A)
		{
			line_nr++;    //detectam finalul de linie
			i++;
		}
		else if(line_nr==line && findall==true) // in cazul in care vrem doar sa validam numarul de linii, nu afisam linia
			break;
		else if(line_nr==line) //mergem pana la linia dorita
		{
			printf("SUCCESS\n");
			int j=i;
			while(j<headers[section_number-1].size) //parcurgem de la inceputul liniei pana la \n sau finalul sectiunii
			{										// si apoi mergem invers poate a afisa caracter cu caracter linia
				if(j==headers[section_number-1].size-1 || ((int)section_buffer[j] == 0x0D && (int)section_buffer[j+1] == 0x0A))
				{
					int k=j;
					while(k>=i)
						printf("%c",section_buffer[k--]);
					break;
				}
				j++;
			}
			printf("\n");
			break;
		}

	free(section_buffer);
	close(file);

	if(line_nr<line)
	{
		if(findall==false)
		{
			printf("ERROR\n");
			printf("invalid line\n");
		}
		return 1;
	}
	return 0;
	
}
//functia contine mai multe booleene pentru a putea fi folosita in mai multe cazuri
void listDir(const char* path, bool recursive,bool perm,bool nsw,const char* start_string,long iteration,bool findall)
{
	DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[1000];
    struct stat statbuf;
    bool write_path=false;

    dir = opendir(path);
    if(dir == NULL) {
        if(iteration==0)
        printf("ERROR\ninvalid directory path\n");
        return;
    }
    else if(iteration==0)
    	printf("SUCCESS\n");

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        	snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
        	if(nsw==true) //functia list name_starts_with
        	{
        		if(strncmp(entry->d_name,start_string ,strlen(start_string))==0) //verificam daca numele elementului incepe cu string-ul cautat
        		{
        			
        			write_path=true;
        		}	
        	}
        	else if(perm==true)//functia list has_perm_write
        	{	
        		
        		if(lstat(fullPath, &statbuf) == 0 && statbuf.st_mode & S_IWUSR)  // https://manpages.ubuntu.com/manpages/kinetic/en/man7/inode.7.html
        			{
						write_path=true;										// verificam daca exista permisiuni de scriere pt owner
        			}       			
        	}
        	else if(findall==true)  //functia findall
        	{
        		if(lstat(fullPath, &statbuf) == 0 && S_ISREG(statbuf.st_mode))
        		{
        			section* headers;
        			int nr_sect=parse(fullPath,true,&headers,0,true);    // verificam daca formatul fisierului e corect, fara printari
        			if(nr_sect>=0)
        			{
        				for(int i=1;i<=nr_sect;i++)
        				{
        					if(extract_section(fullPath,headers,i,14,true)==0) //verificam daca am putea extrage a 13-a linie
        					{
        						write_path=true;
        						free(headers);
        						break;
        					}
        				}
        				if(write_path==false)
        					free(headers); 
        			}		
        		}	
        	}
        	else 
        	{
        		write_path=true;  //in cazul in care functia se apeleaza fara boolene adica doar functionalitatea de baza, mereu afisam tot
        	}
            if(write_path==true)
            {
            	printf("%s\n",fullPath);
            	write_path=false;
            }
            if(lstat(fullPath, &statbuf) == 0) {
                if(S_ISDIR(statbuf.st_mode)) {
                    if(recursive==true)
                    	listDir(fullPath,recursive,perm,nsw,start_string,iteration+1,findall);
                } 
            }
        }
    }
    closedir(dir);
}


int main(int argc, char **argv){
    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("70782\n");
        }

        else if(strcmp(argv[1], "list") == 0)
        {
        	bool recursive=false;
        	bool nsw=false;
        	bool perm=false;
        	char path[1000];                           
        	char start_string[200];
        	//setam argumentele deoarece acestea pot fi in oricare ordine
        	for(int i=1;i<argc;i++)
        	{
        		if(strcmp(argv[i], "recursive")==0)
        			recursive=true;
        		else if(strncmp(argv[i],"name_starts_with=",16)==0)
        		{
        			nsw=true;
        			sprintf(start_string,"%s",argv[i]+17);
        		}
        		else if(strncmp(argv[i],"has_perm_write",14)==0)
        		{
        			perm=true;
        		}
        		else if(strncmp(argv[i],"path",4)==0)
        		{
        
        			sprintf(path,"%s",argv[i]+5);
        		}
        	}
        	
        		listDir(path,recursive,perm,nsw,start_string,0,false);
        }

        else if(strcmp(argv[1], "parse") == 0)
        {
        	char path[1000];

        	sprintf(path,"%s",argv[2]+5);

        	parse(path,false,NULL,0,false);  //facem parsarea cu afisari
        }

        else if(strcmp(argv[1], "extract") == 0)
        {
        	char path[1000];
        	int line;
        	int section_nr;
        	section* headers;

        	sprintf(path,"%s",argv[2]+5);
        	sscanf(argv[4]+5,"%d",&line);
        	sscanf(argv[3]+8,"%d",&section_nr);

        	int res=parse(path,true,&headers,section_nr,false); //parsam doar pentru validare

        	if(res==-1)
        	{
        		printf("ERROR\n");
        		printf("invalid file\n");
        	}
        	else if(res==-2)
        	{
        		printf("ERROR\n");
				printf("invalid section\n");
        		free(headers);
        	}
        	else
        	{
        		extract_section(path,headers,section_nr,line,false); //extragem linia din sectiunea dorita
        		free(headers);
        	}
        }
         else if(strcmp(argv[1], "findall") == 0)
         {
         	char path[1000];

        	sprintf(path,"%s",argv[2]+5);

        	listDir(path,true,false,false,NULL,0,true); //afisam recursiv cu optiunea findall
         }


    }
    return 0;
}