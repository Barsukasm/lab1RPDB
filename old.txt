#include "sqlext.h"
#include "sqltypes.h"
#include "sql.h"
#include "odbcinst.h"
#include <iostream>

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
   retcode = SQLExecDirect(hstmt,  (SQLCHAR*)"SELECT current_database()",   SQL_NTS);
   retcode = SQLFetch(hstmt);
   
   retcode = SQLGetData(hstmt, 1, SQL_C_CHAR, buf, 50, &buf_len);
   if (SQL_SUCCEEDED(retcode)){
      std::string sBuf((char*)buf);
      std::cout<< sBuf<<std::endl;
   } else {
      std::cout << "Не получилось" << std::endl;
   }

   SQLCloseCursor(hstmt);

   retcode = SQLExecDirect(hstmt, (SQLCHAR*)"select * from sample", SQL_NTS);
   if (SQL_SUCCEEDED(retcode)){
      std::cout<<"Получилось"<<std::endl;
   } else {
      extract_error("ExecDirect extract data from sample table", hstmt,SQL_HANDLE_STMT);
   }
   retcode = SQLFetch(hstmt);
   
   SQLINTEGER numb;
   SQLCHAR text[50];
   retcode = SQLGetData(hstmt, 1, SQL_INTEGER, &numb, sizeof(numb), &buf_len);
   if (SQL_SUCCEEDED(retcode)){
      std::cout<< numb<<std::endl;
   } else {
      extract_error("ExecDirect extract data from column 1", hstmt,SQL_HANDLE_STMT);
   }
   retcode = SQLGetData(hstmt, 2, SQL_C_CHAR, text, sizeof(text), &buf_len);
   if (SQL_SUCCEEDED(retcode)){
      std::string sText((char*)text);
      std::cout<< sText<<std::endl;
   } else {
      extract_error("ExecDirect extract data from column 2", hstmt,SQL_HANDLE_STMT);
   }
   SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
   SQLFreeHandle(SQL_HANDLE_ENV, henv);
}
