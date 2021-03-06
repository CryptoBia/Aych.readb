/**
   @file      rdBerkeleyDB.c
   @author    Mitch Richling <https://www.mitchr.me/>
   @Copyright Copyright 1998 by Mitch Richling.  All rights reserved.
   @brief     How to read a Berkely DB.@EOL
   @Keywords  berkely DB dbm ndbm gdbm
   @Std       C99              

   @Tested    
              - Solaris 2.8
              - MacOS X.2
              - Linux (RHEL)
*/

#include "readdb.h"

#define FILENAME "wallet.dat"
#define DATABASE "main"


using namespace std;
//using namespace boost;


int keycnt=0;
int damkeycnt=0;
int namecnt=0;
int poolcnt=0;
int txcnt=0;

// base58
char* B58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

int main(int argc, char *argv[])
{
  DB *myDB; 
  int dbRet, i, maxErr;
  DBT key, value;
  DBC *myDBcursor;

  /* Create the DB */
  if ((dbRet = db_create(&myDB, NULL, 0)) != 0) { // Open failure
    fprintf(stderr, "db_create: %s\n", db_strerror(dbRet)); 
    exit (1); 
  } else {
    printf("DB handle created.\n");
  }

  /* Associate DB with a file (create a btree) -- don't create it!*/
  if ((dbRet = myDB->open(myDB, NULL, FILENAME, DATABASE, DB_BTREE, 0, 0)) != 0)
  { 
    myDB->err(myDB, dbRet, "wallet.dat"); 
    exit(1); // Should close DB now, but we don't cause we are just gonna exit
  }
  else
  {
    printf("DB file opened.\n");
  } 

  /* Get a cursor into the DB */
  dbRet = myDB->cursor(myDB, NULL, &myDBcursor, 0);
  switch(dbRet) {
  case 0: 
    printf("DB cursor initialized.\n");
    break;
  default:
    myDB->err(myDB, dbRet, "DB->cursor");
    exit(1); // Should close DB now, but we don't cause we are just gonna exit
  }

  /* Traverse DB and print what we find.. */
  printf("All the records in the DB:\n");
  i=0;
  maxErr=100;
  zeroDBT(&key);
  zeroDBT(&value);

  while((dbRet = myDBcursor->c_get(myDBcursor, &key, &value, DB_NEXT)) == 0)
  {
    i++;
    switch(dbRet)
    {
      case 0:
//    if(i<100)
        grokData((char*)key.data,(char*)value.data);
//          hdump((char*) key.data, (char*) value.data);
        break;
      case DB_NOTFOUND:
        printf("  Hmmm.. Record not found..\n");
        maxErr--;
        if(maxErr <= 0) {
          printf("  Too many errors.  I'm gonna give up now!\n");
          exit(1);
        }
        break;
      default:
        myDB->err(myDB, dbRet, "  DBcursor->get");
        exit(1);
    }
  }
  printf("Found %d records\n", i);
  printf("encountered %d errors\n", 100-maxErr);
  printf("%i keys found\n",keycnt);
  printf("%i damaged keys found\n",damkeycnt);
  printf("%i addresses found\n",namecnt);
  printf("%i pool reserved found\n",poolcnt);
  printf("%i tx found\n",txcnt);

  /* We should always close our DB -- even before we exit.. */
  closeDB(myDB);
  printf("DB closed... Bye!\n");

  exit(1);
}

void closeDB(DB *dbp) {
  int dbRet;
  dbRet = dbp->close(dbp, 0);
  switch(dbRet) {
  case 0:
    break;
  default:
    printf("Fail: Could not close the db...\n");
  }
}

void zeroDBT(DBT *dbt)
{
  memset(dbt, 0, sizeof(DBT));  
}


void hdump(char* key,char* value)
{
  string skey=key;
  string svalue=value;
  HexDump(key,skey.length());
  if(svalue.length()>1)
    HexDump(value,svalue.length());

}

void grokData(char* key,char* value)
{
  int keylen=key[0];
  string skey=key;
  int valuelen=value[0];
  string svalue=value;
  string address;
  string valuedata;

  int dump=1;

  if(keylen==2)
  {
    string keydata=skey.substr(1,2);
    if(keydata == "tx")
    {
      txcnt++;
      dump=0;
    }
  }
  if(keylen==3)
  {
    string keydata=skey.substr(1,3);
    if(keydata == "key")
    {
      string key="";
      keycnt++;
      valuedata=svalue.substr(1,valuelen);
      int sublen=(unsigned char)skey[4];
      for(int t=0;t<sublen-1;t++)
      {
        unsigned char aa=int(skey[6+t]);
        key=key+toHex(aa);
      }
      if(skey.length()<38)
      {
        int diff=skey.length()-6;
        key=key.substr(0,diff*2);
        printf("*damaged key -- %s\n",key.c_str());
        damkeycnt++;
      }
      else 
        printf("key %s\n",key.c_str());
    string hstring=HexString(value,svalue.length());
        printf("value %s\n",hstring.c_str());
    dump=0;
    }
  }

  if(keylen==4)
  {// check for 'name'
    string keydata=skey.substr(1,4);
    if(keydata == "name")
    {
      namecnt++;
      address=skey.substr(6,34);
      //////////////
      //  check values
      if(valuelen>0)
      {
//    printf("value length=%i \n",valuelen);
        valuedata=svalue.substr(1,valuelen);
//    printf(" value %s\n",valuedata.c_str());
      }
      else
      {
        valuedata="<<missing>>";
      }
//printf("user %s  address %s\n",valuedata.c_str(),address.c_str());
printf("address %s  user %s\n",address.c_str(),valuedata.c_str());
dump=0;
    }
    else if(keydata == "pool")
    {
      poolcnt++;
dump=0;
    }
    else
    {// != 'name'
    printf(" keyname %s\n",keydata.c_str());
    }
  }

  if(dump==1)
  {
//printf("%s  %s\n",skey.c_str(),svalue.c_str());
//printf("key length=%i \n",keylen);
    HexDump(key,skey.length());
  }
}

void HexDump(char *pBuffer, int size)
{
  printf("HexDump output:%i   chars [0x%04x]\n",size,size);
  string c;
  string ct="";
  c="      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  0123456789ABCDEF";
  printf("%s\n",c.c_str());
  c="----  -----------------------------------------------------------------";
  printf("%s\n",c.c_str());

  for(int i=0;i<size;i=i+16)
  {
    ct=printf("%04x ",i);
c=ct.substr(1,2);

    for(int tt=0;tt<16;tt++)
    {
      if(tt+i<size)
      {
        unsigned char cc=int(pBuffer[i+tt]);
        c=c+" "+toHex(cc);
      }
      else
        c=c+"   ";
    }
    c=c+"  ";
    for(int tt=0;tt<16;tt++)
    {
      if(tt+i<size)
      {
        char cc=int(pBuffer[i+tt]);
        if(cc<32)
          cc=46; // make it a dot
        c=c+cc;
      }
    }
  printf("%s\n",c.c_str());
  c="";
  }
  c="-----------------------------------------------------------------------";
  printf("%s\n",c.c_str());
}

string HexString(char *pBuffer, int size)
{
  string c="";
  for(int t=0;t<size;t++)
  {
    unsigned char cc=int(pBuffer[t]);
    c=c+toHex(cc);
  }
  return c;
}

string toHex(unsigned int n)
{
  stringstream ss;
  ss<<hex<<n;
  string sstemp=ss.str();
  if(sstemp.length()==1)
    sstemp="0"+sstemp;

  if(sstemp.length()>2)
printf("error in toHex: length is %i  int was %i\n",sstemp.length(),n);

  return sstemp;
}
