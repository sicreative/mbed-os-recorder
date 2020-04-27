/*
 * json.h
 *
 *  Created on: 12 Apr 2020
 *      Author: slee
 */

#ifndef JSON_H_
#define JSON_H_

#include <map>
#include <string> 
using namespace std;


#define AWS_JSON_NONE 0
#define AWS_JSON_STRING 1
#define AWS_JSON_INT_NUM 2
#define AWS_JSON_CHILD 3
#define json_type int

struct JSON{

	json_type type = AWS_JSON_NONE;
union  {
    int i;
    string* s;
    map<string,JSON>* m;



}value;



~JSON(){

	   if (type==AWS_JSON_STRING){
	                delete value.s;

	                return;
	        }
	        if (type==AWS_JSON_CHILD){
	                delete value.m;

	                return;
	        }

/*
	if (type==AWS_JSON_STRING){
			delete value.s;
			return;
	}
	if (type==AWS_JSON_CHILD){
			delete value.m;
			return;
	}*/

}


};

/*

struct JSON{

    string s;
    map<string,JSON> m;

};*/
string json_writer(map<string,JSON> *m);
map<string,JSON>* json_parse(string& text);

#endif /* JSON_H_ */
