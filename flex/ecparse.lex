%{
/* need this for the call to getlogin() below */
#include <unistd.h>
%}

OperationRegionID OperationRegion
LeftParen \(
RightParen \)

%%
{OperationRegionID} printf("found operation Id\n");
{LeftParen} printf("found \(\n");

%%

int main(){
    yylex();
}