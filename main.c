#include <stdio.h>
#include <stdlib.h>
#include <cJSON.h>
#include <string.h>
#include "config.h"
//RFID keycard coprocesser utility.  
//In the future, will interact with an Arduino that talks to an RFID keycard reader to query our database and unlock the door for valid members who are up-to-date on their dues.
//cJSON.h (and cJSON.c) unedited version of David Gamble's GitHub utility to parse JSON messages


int checkUser(char *userURL)
{
    cJSON *root = NULL;
    char *keycode=NULL;
    char *userID=NULL;
    char *userName=NULL;
    char *userDisplayName=NULL;
    char *userPaidThrough=NULL;
    char *currentDate=NULL;
    int  isValid=0;
    int callfail=0;
    char *code=NULL;
    char *message=NULL;
    char *data=NULL;
    int errorLevel=0;
    int usererror=0;
    int servererror=0;

    char curlURL[200];
    snprintf(curlURL,sizeof curlURL,"%s%s%s",curl_HEADER,userURL,AUTH_HEADER);
    FILE *pp;
    pp = popen(curlURL,"r");
    if(pp != NULL)
    {
        while(1)
        {
            char *line;
            char buf[1000];
            line=fgets(buf,sizeof buf,pp);
            if(line == NULL) break;
            if(line[0]=='{')
            {
                root = cJSON_Parse(line);
                if(!root)
                {
                    printf("Error before: [%s]\n",cJSON_GetErrorPtr());
                }
                else
                {
                    printf("Got JSON\n");
                    //out = cJSON_Print(json);
                    //checks whether we get a good return on the keycard
                    errorLevel=0;
                    callfail=cJSON_HasObjectItem(root,"keycode");
                    if(callfail==1)
                    {
                        keycode = cJSON_GetObjectItem(root,"keycode")->valuestring;
                        printf("keycode = %s\n",keycode);
                    }
                    else
                    {
                        errorLevel+=1;
                    }

                    callfail=cJSON_HasObjectItem(root,"user_id");
                    if(callfail==1)
                    {
                        userID = cJSON_GetObjectItem(root,"user_id")->valuestring;
                        printf("user ID = %s\n",userID);
                    }
                    else
                    {
                        errorLevel+=2;
                    }

                    callfail=cJSON_HasObjectItem(root,"username");
                    if(callfail==1)
                    {
                        userName = cJSON_GetObjectItem(root,"username")->valuestring;
                        printf("User Name = %s\n",userName);
                    }
                    else
                    {
                        errorLevel+=4;
                    }

                    callfail=cJSON_HasObjectItem(root,"user_display_name");
                    if(callfail==1)
                    {
                        userDisplayName = cJSON_GetObjectItem(root,"user_display_name")->valuestring;
                        printf("User Display name = %s\n",userDisplayName);
                    }
                    else
                    {
                        errorLevel+=8;
                    }

                    callfail=cJSON_HasObjectItem(root,"user_paid_through");
                    if(callfail==1)
                    {
                        userPaidThrough = cJSON_GetObjectItem(root,"user_paid_through")->valuestring;
                        printf("User Paid Through = %s\n",userPaidThrough);
                    }
                    else
                    {
                        errorLevel+=16;
                    }

                    callfail=cJSON_HasObjectItem(root,"current_date");
                    if(callfail==1)
                    {
                        currentDate = cJSON_GetObjectItem(root,"current_date")->valuestring;
                        printf("Current Date = %s\n",currentDate);
                    }
                    else
                    {
                        errorLevel+=32;
                    }

                    callfail=cJSON_HasObjectItem(root,"is_valid");
                    if(callfail==1)
                    {
                        isValid = cJSON_GetObjectItem(root,"is_valid")->valueint;
                        printf("Valid User = %d\n",isValid);
                    }
                    else
                    {
                        errorLevel+=64;
                    }

                    callfail=cJSON_HasObjectItem(root,"code");
                    if(callfail==1)
                    {
                    //printf("call didn't fail\n");
                        code = cJSON_GetObjectItem(root,"code")->valuestring;
                        printf("code = %s\n",code);
                    }
                    else
                    {
                        //printf("No Code Returned\n");
                        errorLevel+=128;
                    }

                    callfail=cJSON_HasObjectItem(root,"message");
                    if(callfail==1)
                    {
                        message = cJSON_GetObjectItem(root,"message")->valuestring;
                        printf("message = %s\n",message);
                    }
                    else
                    {
                        errorLevel+=256;
                    }

                    callfail=cJSON_HasObjectItem(root,"data");
                    if(callfail==1)
                    {
                        data = cJSON_GetObjectItem(root,"data")->valuestring;
                        printf("data = %s\n",data);
                     }
                    else
                    {
                        errorLevel+=512;
                    }
                    usererror=0x7F & errorLevel;
                    servererror=0x380 & errorLevel;
                   printf("user card errors = %d\n",usererror);
                   printf("Server Response errors = %d\n",servererror);
                    cJSON_Delete(root);
                    //printf("%s\n",out);
                    //free(out);

                    //printf("%s",line);
                }
            }
        }
    }
    pclose(pp);
 if(isValid==1)
 {
    return 1;
 }
 else
 {
    return -errorLevel;
 }
}


int main()
{
    int testuser=0;
    printf("Try First URL\n");
    testuser=checkUser(Keycard1);
    printf("Response= %d\n",testuser);
    testuser=checkUser(Keycard2);
    printf("Response= %d\n",testuser);
    testuser=checkUser(Keycard3);
    printf("Response= %d\n",testuser);
    testuser=checkUser(Keycard4);
    printf("Response= %d\n",testuser);

    return 0;
}
