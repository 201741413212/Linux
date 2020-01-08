#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>

#define MAX 1024


int ans=0;

int  dp[MAX][MAX]= {0,};

char a[MAX];

char b[MAX];


void LCS()

{

	int result = 0;

	int sLength = strlen(a);

	int tLength = strlen(b);

	for (int i = 0; i < sLength; i++)

	{

		for (int k = 0; k < tLength; k++)

		{

			if (a[i] == b[k])

			{

				if (i == 0 || k == 0)

				{

					dp[i][k] = 1;

				}

				else

				{

					dp[i][k] = dp[i - 1][k - 1] + 1;

				}

				if(dp[i][k]>result)result=dp[i][k];

			}

			else

			{

				dp[i][k] = 0;

			}

		}

	}

	ans=result;

}



void tran()

{

	int sl = strlen(a);

	int tl = strlen(b);

	for(int i=sl-1; i>=0; i--)

	{

		for(int j=tl-1; j>=0; j--)

		{

			if(dp[i][j]==ans&&dp[i-1][j-1]>0)dp[i-1][j-1]=ans;

		}

	}



}



void Print(char *buf)

{

	

	

  //切割字符串

  char str[MAX];

  

  sscanf(buf, "%s",str);

  int fa=0;

  int f=0;

  int fb=0;

  int i=0,j=0,k=0;

  for(i=0;i<strlen(str);i++)

  { 

	  if(str[i]=='='&&!f)fa=1;

      if(str[i]=='&')f=1;

	  if(str[i]=='='&&f)fb=1; 

      if(fa&&!f&&str[i]!='=')a[j++]=str[i];	  

      if(fb&&str[i]!='=')b[k++]=str[i];	  

  }

  a[j]='\0';

  b[k]='\0';

  

  

  LCS();

  tran();



  printf("<!DOCTYPE HTML>\n");



  printf("<html>\n");



  printf("<head>\n");



  printf( "<meta charset=\"utf-8\" />\n");



  printf("</head>\n");



  printf("<body>\n");

  printf("<h1 >");

  printf(" <center>\n");



  printf("a: ");



  for(int i=0; i<strlen(a); i++)

	{

		int flag=1;

		for(int j=0; j<strlen(b); j++)

		{

			if(dp[i][j]==ans)

			{

				printf("<a href="">%c</a>",a[i]);

				flag=0;

			}

			

		}

		if(flag) printf("%c",a[i]);

	}





printf("<br>b: ");



  for(int j=0; j<strlen(b);j++)

	{

		int flag=1;

		for(int i=0; i<strlen(a); i++)

		{

			if(dp[i][j]==ans)

			{

				printf("<a href=""> %c</a>",b[j]);

				flag=0;

			}

		}

		if(flag) printf("%c",b[j]);

	}



  printf("<br>最长公共子串: ");



  for(int i=0; i<strlen(a); i++)

	{

		for(int j=0; j<strlen(b); j++)

		{

			if(dp[i][j]==ans)printf("%c",a[i]);

		}

	}

  printf(" <a href=\"http://127.0.0.1:8080/index.html\"><h3>返回</h3></a>");



  printf(" </center>\n");

printf("</h1>");

  printf("</body>\n");



  printf("</html>\n");







}







int main()



{



  char buff[MAX]={0};



  if(getenv("METHOD"))



  {



    if(strcasecmp(getenv("METHOD"),"GET") == 0)



    {



      strcpy(buff, getenv("QUERY_STRING"));



    }



    else



    {



     int content_length = atoi(getenv("CONTENT_LENGTH"));



     int i = 0 ;



     char c;



     



     for(; i < content_length; ++i)



     {



        read(0, &c, 1);



        buff[i] = c;



     }



     buff[i] = '\0';



    }



  }







  Print(buff);



  return 0;







}



