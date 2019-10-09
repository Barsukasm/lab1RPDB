#include "sqlext.h"
#include "sqltypes.h"
#include "sql.h"
#include "odbcinst.h"
#include <iostream>
#include "string.h"

void extract_error(char *fn, SQLHANDLE handle, SQLSMALLINT type){
   SQLINTEGER i = 0, native;
   SQLCHAR state[7], text[256];
   SQLSMALLINT len;
   SQLRETURN ret;

   std::cout<<stderr<<std::endl<<"The diver reported following diagnostics whilst running "<<fn<<std::endl;
   do
   {
      ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len);
      if(SQL_SUCCEEDED(ret)){
         std::cout<<state<<":"<<i<<":"<<native<<":"<<text<<std::endl;
      }
   } while (ret == SQL_SUCCESS);
   
}

SQLRETURN create_table_inventory(SQLHANDLE handle){
   SQLCHAR *text = (SQLCHAR*)"CREATE TABLE inventory (id serial, size integer NOT NULL, type char(15) NOT NULL, date_prod date);";
   return SQLExecDirect(handle, text, SQL_NTS);
}

bool check_table_existion(SQLHANDLE handle, SQLSMALLINT type){
   SQLINTEGER i = 0, native;
   SQLCHAR state[7], text[256];
   SQLSMALLINT len;
   SQLRETURN ret;

   do
   {
      ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len);
      if(SQL_SUCCEEDED(ret)){
         if(strcmp((char*)state,"42P01") == 0){
            return false;
         } 
      }
   } while (ret == SQL_SUCCESS);
   return true;
}

int main(int argc, char const *argv[])
{
   SQLHENV     henv;     	// Дескриптор окружения
   SQLHDBC     hdbc; 		// Дескриптор соединения
   SQLHSTMT    hstmt; 	// Дескриптор оператора
   SQLRETURN   retcode; 	// Код возврата
   SQLCHAR     buf[50], col2[50];	//
   SQLINTEGER col1[50];
   SQLLEN buf_len;


   retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
   retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
   retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc); 
   retcode = SQLConnect(hdbc, (SQLCHAR*) "PostgreStudy", SQL_NTS,
                        (SQLCHAR*) "zavhoz", SQL_NTS,  (SQLCHAR*) "12345", SQL_NTS); 
   retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); 
   
   retcode = SQLExecDirect(hstmt, (SQLCHAR*)"select * from inventory", SQL_NTS);
   if (SQL_SUCCEEDED(retcode)){
      std::cout<<"Получилось"<<std::endl;
   } else {
      if(check_table_existion(hstmt, SQL_HANDLE_STMT)){
         extract_error("ExecDirect extract data from inventory table", hstmt,SQL_HANDLE_STMT);
      } else {
         if(SQL_SUCCEEDED(create_table_inventory(hstmt))){
            std::cout<<"Создана таблица inventory"<<std::endl;
         } else{
            extract_error("ExecDirect create inventory table", hstmt,SQL_HANDLE_STMT);
         }
      }
   }

   SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
   SQLFreeHandle(SQL_HANDLE_ENV, henv);
}
