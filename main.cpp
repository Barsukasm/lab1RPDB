#include "sqlext.h"
#include "sqltypes.h"
#include "sql.h"
#include "odbcinst.h"
#include <iostream>
#include "string.h"
#include <ctime>

//Вывод ошибок
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

//Функции для проверки существования и создания таблиц
SQLRETURN create_table_inventory(SQLHANDLE handle){
   SQLCHAR *text = (SQLCHAR*)"CREATE TABLE inventory (id serial PRIMARY KEY, size integer NOT NULL, type char(15) NOT NULL, date_prod date);";
   return SQLExecDirect(handle, text, SQL_NTS);
}

SQLRETURN create_table_clients(SQLHANDLE handle){
   SQLCHAR *text = (SQLCHAR*)"CREATE TABLE clients (id serial PRIMARY KEY, first_name char(20) NOT NULL, last_name char(20) NOT NULL);";
   return SQLExecDirect(handle, text, SQL_NTS);
}

SQLRETURN create_table_rents(SQLHANDLE handle){
   SQLCHAR *text = (SQLCHAR*)"CREATE TABLE rents (id_item integer references inventory(id), rent_start date NOT NULL, rent_end date, id_client integer references clients(id));";
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

//Базовые функции
void watch_records(SQLHANDLE handle){
   SQLCloseCursor(handle);
   SQLCHAR* text = (SQLCHAR*)"select rents.id,rents.id_item,rents.rent_start,rents.rent_end,rents.id_client,inventory.size,inventory.type from inventory,rents where rents.id_item=inventory.id;";
   SQLRETURN ret;
   ret = SQLExecDirect(handle,text,SQL_NTS);
   while (SQL_SUCCEEDED(ret = SQLFetch(handle)))
   {
      SQLINTEGER id,id_item,id_client,size;
      SQLLEN ind[7];
      SQLCHAR rent_start[50],rent_end[50],type[15];
      ret = SQLGetData(handle,1,SQL_INTEGER,&id,sizeof(id),&ind[0]);
      if (SQL_SUCCEEDED(ret)){
         std::cout<<id<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,2,SQL_INTEGER,&id_item,sizeof(id_item),&ind[1]);
      if (SQL_SUCCEEDED(ret)){
         std::cout<<id_item<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,3,SQL_C_CHAR,rent_start,sizeof(rent_start),&ind[2]);
      if (SQL_SUCCEEDED(ret)){
         if(ind[2] == SQL_NULL_DATA) strcpy((char*)rent_start, "NULL");
         std::cout<<rent_start<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,4,SQL_C_CHAR,rent_end,sizeof(rent_end),&ind[3]);
      if (SQL_SUCCEEDED(ret)){
         if(ind[3] == SQL_NULL_DATA) strcpy((char*)rent_end, "NULL");
         std::cout<<rent_end<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,5,SQL_INTEGER,&id_client,sizeof(id_client),&ind[4]);
      if (SQL_SUCCEEDED(ret)){
         std::cout<<id_client<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,6,SQL_INTEGER,&size,sizeof(size),&ind[5]);
      if (SQL_SUCCEEDED(ret)){
         std::cout<<size<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,7,SQL_C_CHAR,type,sizeof(type),&ind[6]);
      if (SQL_SUCCEEDED(ret)){
         if(ind[6] == SQL_NULL_DATA) strcpy((char*)type, "NULL"); 
         std::cout<<type<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      std::cout<<std::endl;
   }
   
   
}

bool add_rent_record(SQLHANDLE handle,int id_item, char *first_name, char *last_name){
   SQLRETURN ret;
   SQLCloseCursor(handle);
   //Проверяем есть ли клиент
   std::string text="select exists (select 1 from clients where last_name='"+std::string(last_name)+"' and first_name='"+std::string(first_name)+"');";
   SQLCHAR q1[512];
   strcpy((char*)q1,text.c_str());
   //std::cout<<(char*)q1<<std::endl;

   ret = SQLExecDirect(handle, q1,SQL_NTS);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Checking exist of client (ExecDirect)",handle,SQL_HANDLE_STMT);
      return false;
   }
   ret = SQLFetch(handle);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Checking exist of client (Fetch)",handle,SQL_HANDLE_STMT);
      return false;
   }
   
   bool check;
   SQLLEN indicator;
   SQLINTEGER id_client;

   ret = SQLGetData(handle,1,SQL_BIT,&check,sizeof(check),&indicator);
   
   if (SQL_SUCCEEDED(ret))
   {
      //Если клиента не существует, то добавяем его
      if(!check){
         std::cout<<"Added new client"<<std::endl;
         SQLCloseCursor(handle);
         std::string createClient = "insert into clients(first_name,last_name) values ('"+std::string(first_name)+"', '"+std::string(last_name)+"');";
         //std::cout<<(char*)q1<<std::endl;
         strcpy((char*)q1,createClient.c_str());
         ret = SQLExecDirect(handle,q1,SQL_NTS);
         if (!SQL_SUCCEEDED(ret))
         {
            extract_error("Creating new client",handle,SQL_HANDLE_STMT);
            return false;
         }
         
      }
      
   } else{
      extract_error("Checking exist of client (GetData)",handle,SQL_HANDLE_STMT);
      return false;
   }
   std::string getIdClient = "select id from clients where first_name='"+std::string(first_name)+"' and last_name='"+std::string(last_name)+"';";
   strcpy((char*)q1,getIdClient.c_str());
   SQLCloseCursor(handle);
   //Получить id клиента
   ret = SQLExecDirect(handle,q1,SQL_NTS);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Getting client id (ExecDirect)",handle,SQL_HANDLE_STMT);
      return false;
   }
   ret = SQLFetch(handle);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Getting client id (Fetch)",handle,SQL_HANDLE_STMT);
      return false;
   }
   ret = SQLGetData(handle,1,SQL_INTEGER,&id_client,sizeof(id_client),&indicator);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Getting client id (GetData)",handle,SQL_HANDLE_STMT);
      return false;
   }
   //std::cout<<"Client id:"<<id_client<<std::endl;
   //Проверить существование арендуемого предмета
   std::string checkInventory = "select exists(select 1 from inventory where id="+ std::to_string(id_item) +");";
   SQLCloseCursor(handle);
   strcpy((char*)q1,checkInventory.c_str());
   ret = SQLExecDirect(handle, q1,SQL_NTS);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Checking exist of item (ExecDirect)",handle,SQL_HANDLE_STMT);
      return false;
   }
   ret = SQLFetch(handle);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Checking exist of item (Fetch)",handle,SQL_HANDLE_STMT);
      return false;
   }

   ret = SQLGetData(handle,1,SQL_BIT,&check,sizeof(check),&indicator);
   if (SQL_SUCCEEDED(ret))
   {
      //Если предмета с указанным номером не существует, то все
      if(!check){        
         std::cout<<"No such item"<<std::endl;
         return false;        
      }
      
   } else{
      extract_error("Checking exist of item (GetData)",handle,SQL_HANDLE_STMT);
      return false;
   }
   //Проверяем свободен ли предмет
   checkInventory = "select exists(select 1 from rents where id_item="+std::to_string(id_item)+" and rent_end is null);";

   SQLCloseCursor(handle);
   strcpy((char*)q1,checkInventory.c_str());
   ret = SQLExecDirect(handle, q1,SQL_NTS);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Checking availability of item (ExecDirect)",handle,SQL_HANDLE_STMT);
      return false;
   }
   ret = SQLFetch(handle);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Checking availability of item (Fetch)",handle,SQL_HANDLE_STMT);
      return false;
   }

   ret = SQLGetData(handle,1,SQL_BIT,&check,sizeof(check),&indicator);
   if (SQL_SUCCEEDED(ret))
   {
      //Если предмет с указанным номером найден, то все. Он занят
      if(check){
         std::cout<<"Item is unavailable atm"<<std::endl;
         return false;        
      }
      
   } else{
      extract_error("Checking availability of item (GetData)",handle,SQL_HANDLE_STMT);
      return false;
   }
   //Добавить запись об аренде
   time_t now;

   time(&now);
   tm *ltm = localtime(&now);
   
   std::string timeString = std::to_string(1900+ltm->tm_year)+"-"+std::to_string(1+ltm->tm_mon)+"-"+std::to_string(ltm->tm_mday);
   //std::cout<<timeString<<std::endl;

   std::string addRecord = "insert into rents values(" + std::to_string(id_item) + ", '"+timeString+"', NULL, "+ std::to_string(id_client) +");";
   strcpy((char*)q1,addRecord.c_str());
   SQLCloseCursor(handle);
   //Непосредственно вставка
   ret = SQLExecDirect(handle,q1,SQL_NTS);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("insert into rents values (SQLExecDirect)",handle,SQL_HANDLE_STMT);
      return false;
   }
   //std::cout<<"Successfully added"<<std::endl;
   return true;
}

bool add_inventory_record(SQLHANDLE handle, int size, char* type, char* date_prod){
   SQLRETURN ret;
   SQLCloseCursor(handle);
   std::string addRecord = "insert into inventory(size,type,date_prod) values("+std::to_string(size)+", '"+type+"', '"+date_prod+"');";
   SQLCHAR q1[512];
   strcpy((char*)q1,addRecord.c_str());
   //std::cout<<(char*)q1<<std::endl;

   ret = SQLExecDirect(handle, q1,SQL_NTS);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("insert into inventory (ExecDirect)",handle,SQL_HANDLE_STMT);
      return false;
   }
   return true;
}

bool change_record(SQLHANDLE handle, int id){
   SQLRETURN ret;
   SQLCloseCursor(handle);
   //Проверим, не закрыта ли уже заявка
   std::string checkInventory = "select exists(select 1 from rents where id="+std::to_string(id)+" and rent_end is null);";
   SQLCHAR q1[512];
   strcpy((char*)q1,checkInventory.c_str());
   ret = SQLExecDirect(handle, q1,SQL_NTS);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Checking availability of item (ExecDirect)",handle,SQL_HANDLE_STMT);
      return false;
   }
   ret = SQLFetch(handle);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("Checking availability of item (Fetch)",handle,SQL_HANDLE_STMT);
      return false;
   }
   bool check;
   SQLLEN indicator;
   ret = SQLGetData(handle,1,SQL_BIT,&check,sizeof(check),&indicator);
   if (SQL_SUCCEEDED(ret))
   {
      //Если предмет с указанным не номером найден, то все. Он закрыт
      if(!check){
         //std::cout<<"Item is already closed"<<std::endl;
         return false;        
      }
      
   } else{
      extract_error("Checking availability of item (GetData)",handle,SQL_HANDLE_STMT);
      return false;
   }
   //По сути просто закрываем аренду по номеру записи
   time_t now;

   time(&now);
   tm *ltm = localtime(&now);
   
   std::string timeString = std::to_string(1900+ltm->tm_year)+"-"+std::to_string(1+ltm->tm_mon)+"-"+std::to_string(ltm->tm_mday);
   //std::cout<<timeString<<std::endl;
   
   SQLCloseCursor(handle);
   std::string addRecord = "update rents set rent_end='"+timeString+"'  where id="+std::to_string(id)+";";
   
   strcpy((char*)q1,addRecord.c_str());
   //std::cout<<(char*)q1<<std::endl;

   ret = SQLExecDirect(handle, q1,SQL_NTS);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("insert into inventory (ExecDirect)",handle,SQL_HANDLE_STMT);
      return false;
   }
   return true;
}

bool delete_record(SQLHANDLE handle, int id){
   //удаляем по id заявки
   SQLRETURN ret;
   SQLCloseCursor(handle);
   std::string addRecord = "delete from rents where id="+std::to_string(id)+";";
   SQLCHAR q1[512];
   strcpy((char*)q1,addRecord.c_str());
   //std::cout<<(char*)q1<<std::endl;

   ret = SQLExecDirect(handle, q1,SQL_NTS);
   if (!SQL_SUCCEEDED(ret))
   {
      extract_error("insert into inventory (ExecDirect)",handle,SQL_HANDLE_STMT);
      return false;
   }
   return true;
}

void search_records(SQLHANDLE handle,char* type, int size){
   SQLCloseCursor(handle);
   std::string q1;
   if (size==0)
   {
      q1 = "select id,size,type from inventory where type like'%"+(std::string)type+"%';";
   } else
   {
      q1 = "select id,size,type from inventory where type like '%"+(std::string)type+"%' and size="+std::to_string(size)+";";
   }
   
   
   
   SQLCHAR text[512];
   strcpy((char*)text, q1.c_str());
   //std::cout<<text<<std::endl;
   SQLRETURN ret;
   std::cout<<"id"<<" \t| ";
   std::cout<<"size"<<" \t| ";
   std::cout<<"type"<<" \t| ";
   std::cout<<std::endl;
   ret = SQLExecDirect(handle,text,SQL_NTS);
   while (SQL_SUCCEEDED(ret = SQLFetch(handle)))
   {
      SQLINTEGER id,id_item,id_client,size;
      SQLLEN ind[3];
      SQLCHAR type[20];
      ret = SQLGetData(handle,1,SQL_INTEGER,&id,sizeof(id),&ind[0]);
      if (SQL_SUCCEEDED(ret)){
         std::cout<<id<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,2,SQL_INTEGER,&size,sizeof(size),&ind[1]);
      if (SQL_SUCCEEDED(ret)){
         std::cout<<size<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,3,SQL_C_CHAR,type,sizeof(type),&ind[2]);
      if (SQL_SUCCEEDED(ret)){
         if(ind[2] == SQL_NULL_DATA) strcpy((char*)type, "NULL"); 
         std::cout<<type<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      std::cout<<std::endl;
   }
}

void watch_equip_age(SQLHANDLE handle){
   SQLCloseCursor(handle);
   SQLCHAR* text = (SQLCHAR*)"select * from watch_age();";
   SQLRETURN ret;
   ret = SQLExecDirect(handle,text,SQL_NTS);
   std::cout<<"id"<<" \t| ";
   std::cout<<"size"<<" \t| ";
   std::cout<<"type"<<" \t| ";
   std::cout<<"age"<<" \t| ";
   std::cout<<std::endl;
   while (SQL_SUCCEEDED(ret = SQLFetch(handle)))
   {
      SQLINTEGER id,age,size;
      SQLLEN ind[4];
      SQLCHAR type[20];
      
      ret = SQLGetData(handle,1,SQL_INTEGER,&id,sizeof(id),&ind[0]);
      if (SQL_SUCCEEDED(ret)){
         std::cout<<id<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,2,SQL_INTEGER,&size,sizeof(size),&ind[1]);
      if (SQL_SUCCEEDED(ret)){
         std::cout<<size<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,3,SQL_C_CHAR,type,sizeof(type),&ind[2]);
      if (SQL_SUCCEEDED(ret)){
         if(ind[6] == SQL_NULL_DATA) strcpy((char*)type, "NULL"); 
         std::cout<<type<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      ret = SQLGetData(handle,4,SQL_INTEGER,&age,sizeof(age),&ind[3]);
      if (SQL_SUCCEEDED(ret)){
         std::cout<<age<<" \t| ";
      } else{
         extract_error("GetData in watch records",handle,SQL_HANDLE_STMT);
      }
      std::cout<<std::endl;
   }
}

//Меню
void print_menu(){
   std::cout<<"1. Просмотр журнала проката"<<std::endl;
   std::cout<<"2. Добавление новой записи журнала проката"<<std::endl;
   std::cout<<"3. Закрытие записи журнала проката"<<std::endl;
   std::cout<<"4. Удаление записи из журнала проката"<<std::endl;
   std::cout<<"5. Поиск снаряжение по типу и размеру"<<std::endl;
   std::cout<<"6. Просмотр износа снаряжение"<<std::endl;
   std::cout<<"7. Добавить новое снаряжение"<<std::endl;
   std::cout<<"0. Выход"<<std::endl;
   std::cout<<"Любое дугое число - вывод данного меню"<<std::endl;
};

int main(int argc, char const *argv[])
{
   SQLHENV     henv;     	// Дескриптор окружения
   SQLHDBC     hdbc; 		// Дескриптор соединения
   SQLHSTMT    hstmt; 	// Дескриптор оператора
   SQLRETURN   retcode; 	// Код возврата
   SQLCHAR     buf[50];	//
   SQLLEN buf_len;


   retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
   retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
   retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc); 
   retcode = SQLConnect(hdbc, (SQLCHAR*) "PostgreStudy", SQL_NTS,
                        (SQLCHAR*) "zavhoz", SQL_NTS,  (SQLCHAR*) "12345", SQL_NTS); 
   retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); 
   
   //Блок проверки существования нужных таблиц. Если они не найдены, то создаем их.
   retcode = SQLExecDirect(hstmt, (SQLCHAR*)"select * from inventory", SQL_NTS);
   if (!SQL_SUCCEEDED(retcode)){      
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
   SQLCloseCursor(hstmt);

   retcode = SQLExecDirect(hstmt, (SQLCHAR*)"select * from clients", SQL_NTS);
   if (!SQL_SUCCEEDED(retcode)){      
      if(check_table_existion(hstmt, SQL_HANDLE_STMT)){
         extract_error("ExecDirect extract data from rents table", hstmt,SQL_HANDLE_STMT);
      } else {
         if(SQL_SUCCEEDED(create_table_clients(hstmt))){
            std::cout<<"Создана таблица clients"<<std::endl;
         } else{
            extract_error("ExecDirect create clients table", hstmt,SQL_HANDLE_STMT);
         }
      }
   }
   SQLCloseCursor(hstmt);

   retcode = SQLExecDirect(hstmt, (SQLCHAR*)"select * from rents", SQL_NTS);
   if (!SQL_SUCCEEDED(retcode)){      
      if(check_table_existion(hstmt, SQL_HANDLE_STMT)){
         extract_error("ExecDirect extract data from rents table", hstmt,SQL_HANDLE_STMT);
      } else {
         if(SQL_SUCCEEDED(create_table_rents(hstmt))){
            std::cout<<"Создана таблица rents"<<std::endl;
         } else{
            extract_error("ExecDirect create rents table", hstmt,SQL_HANDLE_STMT);
         }
      }
   }
   SQLCloseCursor(hstmt);
   //Конец блока проверок
   //watch_records(hstmt);\/
   //bool i = add_rent_record(hstmt,1,"Ivan","Ivanov");\/
   //bool i = change_record(hstmt,1);\/
   //bool i = delete_record(hstmt, 13);\/
   //watch_equip_age(hstmt);\/
   //search_records(hstmt,"snowboard",44);\/
   //Меню и реализация функционала
   bool exit = false;
   int r;
   print_menu();
   while (!exit)
   {      
      std::cin>>r;
      switch (r)
      {
      case 0:
         {
            exit=true;
         }
         break;
      case 1:
         {
            watch_records(hstmt);
         }
         break;
      case 2:
         {
            std::cout<<"Введите номер предмета:"<<std::endl;
            int id;
            std::string first_name, last_name;
            std::cin>>id;
            std::cout<<"Введите фамилию:"<<std::endl;
            std::cin>>last_name;
            std::cout<<"Введите имя:"<<std::endl;
            std::cin>>first_name;
            bool i = add_rent_record(hstmt,id,(char*)first_name.c_str(), (char*)last_name.c_str());
            if (i)
            {
               std::cout<<"Успешно добавлено"<<std::endl;
            }else{
               std::cout<<"Данные введены некорректно"<<std::endl;
            }
            
         }
         break;
      case 3:
         {
            std::cout<<"Введите номер записи:"<<std::endl;
            int id;
            std::cin>>id;
            bool i = change_record(hstmt,id);
            if (i)
            {
               std::cout<<"Успешно возвращено"<<std::endl;
            }else{
               std::cout<<"Данные введены некорректно"<<std::endl;
            }
         }
         break;
      case 4:
         {
            std::cout<<"Введите номер записи:"<<std::endl;
            int id;
            std::cin>>id;
            bool i = delete_record(hstmt, id);
            if (i)
            {
               std::cout<<"Успешно удалено"<<std::endl;
            } else{
               std::cout<<"Данные введены некорректно"<<std::endl;
            }
         }
         break;
      case 5:
         {            
            int size;
            std::string type;
            std::cout<<"Введите размер:"<<std::endl;
            std::cin>>size;
            std::cout<<"Введите тип желаемого снаряжения:"<<std::endl;
            std::cin>>type;

            
            search_records(hstmt,(char*)type.c_str(),size);
         }
         break;
      case 6:
         {
            watch_equip_age(hstmt);
         }
         break;
      case 7:
         {
            int size;
            std::string type, date_prod;
            std::cout<<"Введите размер:"<<std::endl;
            std::cin>>size;
            std::cout<<"Введите тип желаемого снаряжения:"<<std::endl;
            if (std::cin.peek()) std::cin.ignore();
            getline(std::cin,type);
            std::cout<<"Введите дату производства снаряжения:"<<std::endl;
            getline(std::cin,date_prod);

            add_inventory_record(hstmt,size,(char*)type.c_str(),(char*)date_prod.c_str());
         }
         break;
      default:
         {
            print_menu();
         }
         break;
      }
   }
   

   SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
   SQLFreeHandle(SQL_HANDLE_ENV, henv);
}
