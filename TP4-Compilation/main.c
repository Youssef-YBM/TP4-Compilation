#include <stdio.h>
#include <stdlib.h>

FILE* code_source=NULL;
 FILE* output =NULL;
int state=0 ;// pour garder l'etat de l'automate
int nb_ligne = 1;
int new_line =  0;

//  Unites Lexicales
typedef enum enu{
	SI,SINON,ALORS,OPREL, ID, NB,SCANEOF
}token;

char tokens[6][9]={"SI","SINON","ALORS","OPREL","ID","NB"};

void recover(char c){
	printf("Ce character: %c est inconnue; --{Verifier Ligne : %d}--\n", c, nb_ligne);
	c = fgetc(code_source);
	state = 0;
}

void fail(char c){
  ungetc(c,code_source);
  switch(state){
      case 0: state=11; break;
      case 11: state = 15; break;
      case 15:state=17; break;
      default : recover(c);
  }
}

char to_min(char c)
{
	return ((c <= 'Z') && (c >= 'A')) ? (c - 'A' + 'a') : c;
}

char next_char()
{
	return (to_min(fgetc(code_source)));
}

int is_space(char c)
{
	return ((c == ' ') || (c == '\t')) ? 1 : (c == '\n') ? (nb_ligne++, new_line = 1, 1) : 0;
}

int lettre(char c)
{
	return (((c <= 'z') && (c >= 'a')) || ((c <= 'Z') && (c >= 'A')));
}

int chiffre(char c)
{
	return ((c <= '9') && (c >= '0'));
}

token next_token()
{
    char c ;
    char c_tmp,c_tmp2;
    state =0;
    while(c!=EOF)
    {
        switch(state)
        {
            case 0 : c=next_char();
                     if(is_space(c)) break;
                     if(c=='s') state=1;
                     else if (c=='a') state=6;
                     else fail(c);
                     break;
            case 1:  c=next_char();
                     if(c=='i') state=2;
                     else if (lettre(c) || chiffre(c)) state = 16;
                     else
                     {
                         ungetc(c,code_source);
                         state=0;
                         return ID;
                     }
                     break;
            case 2:  c=next_char();
                      if(c=='n') state=3;
                      else if(lettre(c) || chiffre(c)) state=16;
                      else
                      {
                          ungetc(c,code_source);
                          state=0;
                          return SI;
                      }
                      break;
            case 3:  c=next_char();
                     if(c=='o') state=4;
                     else if(lettre(c) || chiffre(c)) state=16;
                     else
                     {
                         ungetc(c,code_source);
                         state=0;
                         return ID;
                     }
                     break;
           case 4:   c=next_char();
                     if(c=='n') state =5;
                     else if(lettre(c) || chiffre(c)) state=16;
                     else
                     {
                         ungetc(c,code_source);
                         state=0;
                         return ID;
                     }
                     break;
           case 5:  c = next_char();
				    if ((lettre(c)) || (chiffre(c))) state = 16;
				    else
				    {
					     ungetc(c, code_source);
					     state=0;
					     return SINON;
				    }
				    break;
           case 6:  c=next_char();
                    if(c=='l') state=7;
                    else if( (lettre(c)) || chiffre(c) )state=16;
                    else
                    {
                        ungetc(c,code_source);
                        state=0;
                        return ID;
                    }
                    break;
          case 7:   c=next_char();
                    if(c=='o') state=8;
                    else if( (lettre(c)) || chiffre(c) )state=16;
                    else
                    {
                        ungetc(c,code_source);
                        state=0;
                        return ID;
                    }
                    break;
          case 8:   c=next_char();
                    if(c=='r') state=9;
                    else if( (lettre(c)) || chiffre(c) )state=16;
                    else
                    {
                        ungetc(c,code_source);
                        state=0;
                        return ID;
                    }
                    break;
          case 9:   c=next_char();
                    if(c=='s') state=10;
                    else if( (lettre(c)) || chiffre(c) )state=16;
                    else
                    {
                        ungetc(c,code_source);
                        state=0;
                        return ID;
                    }
                    break;
          case 10:  c = next_char();
				    if ((lettre(c)) || (chiffre(c))) state = 16;
				    else
				    {
					     ungetc(c, code_source);
					     state=0;
					     return ALORS;
				    }
				    break;
		  case 11:  c=next_char();
                    if(c=='<') state=12;
                    else if(c=='>') state=13;
                    else if(c=='=') state=14;
                    else fail(c);
                    break;
          case 12:  c=next_char();
                    if(c=='>' || c=='=') state=14;
                    else
                    {
                        ungetc(c,code_source);
                        state=0;
                        return OPREL;
                    }
                    break;
          case 13:  c=next_char();
                    if(c=='=') state=14;
                    else
                    {
                        ungetc(c,code_source);
                        state=0;
                        return OPREL;
                    }
                    break;
          case 14: return OPREL;
          case 15: c=next_char();
                   if(lettre(c)) state=16;
                   else fail(c);
                   break;
          case 16: c = next_char();
			       if (!(lettre(c)) && !(chiffre(c)))
			        {
				      ungetc(c,code_source);
				      state=0;
				      return ID;
			        }
			        break;
           case 17: c=next_char();
                    if (chiffre(c)) state = 18;
                    else fail(c);
			        break;
		  case 18:  c = next_char();
			        if (chiffre(c)) break;
			        if (c == '.') state = 19;
			        else if ((c == 'E') || (c == 'e'))
			        {
				       c_tmp = c;
				       state = 20;
			        }
                    else
			        {
                        ungetc(c,code_source);
                        state=0;
                        return NB;
                    }
                    break;
		  case 19:  c = next_char();
			        if (chiffre(c)) state = 21;
			        else
			        {
                        ungetc(c, code_source);
                        ungetc('.',code_source);
                        state=0;
                        return NB;
			        }
			        break;
		  case 20:  c = next_char();
			        if ((c == '+') || (c == '-'))
			        {
				      c_tmp2 = c;
				      state = 23;
			        }
			        else if (chiffre(c)) state = 22;
			        else
			        {
				       ungetc(c, code_source);
				       ungetc(c_tmp,code_source);
				       state=0;
				       return NB;
                    }
                    break;
		  case 21:  c = next_char();
                    if (chiffre(c)) break;
                    if ((c == 'E') || (c == 'e'))
                    {
                        c_tmp = c;
                        state = 20;
                    }
                    else
                    {
                        ungetc(c,code_source);
                        state=0;
                        return NB;
                    }
                    break;
          case 22:  c = next_char();
			        if (chiffre(c)) break;
			        ungetc(c,code_source);
			        state=0;
			        return NB;
		  case 23:  c = next_char();
			       if (chiffre(c)) state = 22;
			       else
                   {
                    ungetc(c, code_source);
                    ungetc(c_tmp2, code_source);
                    ungetc(c_tmp,code_source);
                    state=0;
                    return NB;
                  }
                    break;
		}

	}
	return SCANEOF;
}


int main(){
	token _token;

	code_source=fopen("input.txt", "r");
	output=fopen("sortie_analyseur.txt", "w");

	if (!code_source  || !output)
	{
		printf("Error! Could not read one or more files\n");
		exit(-1);
	}

    while ((_token = next_token()) != SCANEOF)
	{
		if (new_line)
		{
			fprintf(output, "%c", '\n');
			new_line = 0;
		}
		fprintf(output, "%s ", tokens[_token]);
	}

    fclose(output);
    fclose(code_source);
    return 0;
}
