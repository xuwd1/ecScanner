
%{
/* need this for the call to getlogin() below */
#include <unistd.h>
%}

%%
username	printf("%s\n", getlogin());
%%

int main()
{
  yylex();
}