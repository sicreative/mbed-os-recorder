/*
 * awsiot.cpp
 *
 *      Author: sc lee
 *
 *  Licensed under the Apache License, Version 2.0
 */

#include "awsiot.h"

#define SHADOW_USE_JSON_MAP 0





/**
 *
 * Handling AWS Activities
 */
#include "mbed.h"
#include "aws_client.h"
#include "aws_config.h"

#include "lcd_ui.h"
#include "json_aws.h"
#include "hw.h"
#include "network_activity_handler.h"



#include <map>





NetworkInterface* _network;

void awsiot_send(void);

// define working with AWS IOT shadow
#define SEND_SHADOW




#define AWSIOT_KEEPALIVE_TIMEOUT            (60)
#define AWS_IOT_SECURE_PORT                 (8883)
#define AWSIOT_TIMEOUT_IN_USEC              (1000UL * 1000UL)
#define AWSIOT_SEND_PERIOD_MS                400








AWSIoTClient *client = NULL;
aws_publish_params_t publish_params;

Thread awsiot_thread(osPriorityAboveNormal, 8196 , NULL, "awsiot_thread");
Semaphore awsiot_sem(1);

#if SHADOW_USE_JSON_MAP
	map<string,JSON> shadow_status_map;
#else
#define SHADOW_STATUS_PREFIX 		"{ \"state\": { \"reported\" : {"
#define SHADOW_STATUS_SUFFIX    	"}}}"
	string shadow_status;
#endif


int awsiot_subscribe_shadow(char* command,subscriber_callback callback);
int awsiot_publish_shadow( const char* message,const char* command);
int awsiot_subscribe(const char* topic,subscriber_callback callback);
int awsiot_unsubscribe_shadow(char* command);
int awsiot_unsubscribe(const char* topic);
int awsiot_publish( const char* message,const char* topic);
void awsiot_update(string name, string value,bool publish);
void awsiot_connect_thread( NetworkInterface* network,bool retry );


void awsiot_subscribe_update_callback( aws_iot_message_t& md){

	string pl = (char*)md.message.payload;


	 client->yield();
	 map<string,JSON>* m = json_parse(pl);


	 	 	 JSON* state = &((*m)["state"]);
	 	 	 map<string,JSON>* sm = state->value.m;
	 	 	if ( sm->find("reported") != sm->end() ) {



	 	 	 JSON* reported = &((*sm)["reported"]);
	 	 	 hw_aws_sync(reported);

	 	 	}


	 	 	if ( sm->find("desired") != sm->end() ) {



	 	 			 	 	 JSON* desired = &((*sm)["desired"]);
	 	 			 	 	 if (hw_aws_sync(desired))
	 	 			 	 		lcd_msg_delay_close("Update Accepted",2,1000);
	 	 		}



	 	 	delete m;

}





void awsiot_subscribe_get_callback( aws_iot_message_t& md){

	string pl = (char*)md.message.payload;
	 aws_message_t &message = md.message;
	 APP_INFO(( "\r\nMessage arrived: qos %d, retained %d, dup %d, packetid %d", message.qos, message.retained, message.dup, message.id ));
     APP_INFO(( "\r\nPayload %.*s\r\n", message.payloadlen, (char*)message.payload ));

     client->yield();

		 map<string,JSON>* m = json_parse(pl);


		 	 	 JSON* state = &((*m)["state"]);
		 	 	 map<string,JSON>* sm = state->value.m;
		 	 	if ( sm->find("reported") != sm->end() ) {



		 	 	 JSON* reported = &((*sm)["reported"]);
		 	 	 hw_aws_sync(reported);


		 	 	}


		 		delete m;


}




int awsiot_shadow_cycle(){


	awsiot_publish_shadow("","/get");

	lcd_msg("AWS shadow working",0);

	while(true){


		awsiot_sem.acquire();

#if SHADOW_USE_JSON_MAP

		if (!shadow_status_map.empty()){

		string message = json_writer(&shadow_status_map);
		int result = awsiot_publish_shadow(message.c_str(),"/update");

		if (result==1)
			shadow_status_map.clear();


		}

#else
		if (!shadow_status.empty()){
			shadow_status.insert(0,SHADOW_STATUS_PREFIX);
			shadow_status.append(SHADOW_STATUS_SUFFIX);
			int result = awsiot_publish_shadow(shadow_status.c_str(),"/update");

					if (result==1)
						shadow_status.clear();


		}


# endif
		awsiot_sem.release();



			cy_rslt_t result = client->yield();



		if ( result == CY_RSLT_AWS_ERROR_DISCONNECTED ){

				APP_INFO(( "Yield Error\n" ));
				if (_network->get_connection_status()!=NSAPI_STATUS_GLOBAL_UP){
					_network->disconnect();
					_network->emacInterface()->get_emac().power_down();
					_network->emacInterface()->get_emac().power_up();
					_network = NULL;
				}
				awsiot_connect_thread(_network,true);
			}








		Cy_SysLib_Delay(AWSIOT_SEND_PERIOD_MS);
	}


}



void awsiot_connect_thread( NetworkInterface* network,bool retry )
{



	aws_connect_params_t conn_params;
	aws_endpoint_params_t endpoint_params;


	while (!network){



		network_connect();

	}


	if (client!=NULL){


		if (network->get_connection_status()==NSAPI_STATUS_DISCONNECTED)
			network->connect();

		client->disconnect();
		retry=true;
		delete(client);
	    client = NULL;
	}


    conn_params = { 0, 0, NULL, NULL, NULL, NULL, NULL };
    publish_params = { AWS_QOS_ATMOST_ONCE };
    endpoint_params = { AWS_TRANSPORT_MQTT_NATIVE, NULL, 0, NULL, 0 };



    cy_rslt_t result = CY_RSLT_SUCCESS;


    _network = network;




    if ( ( strlen(SSL_CLIENTKEY_PEM) | strlen(SSL_CLIENTCERT_PEM) | strlen(SSL_CA_PEM) ) < 64 )
    {
    	   lcd_msg("aws_config.h error",0);

        APP_INFO(( "Please configure SSL_CLIENTKEY_PEM, SSL_CLIENTCERT_PEM and SSL_CA_PEM in aws_config.h file\n" ));
        return ;
    }

    /* Initialize AWS Client library */
    client = new AWSIoTClient( network, AWSIOT_THING_NAME, SSL_CLIENTKEY_PEM, strlen(SSL_CLIENTKEY_PEM), SSL_CLIENTCERT_PEM, strlen(SSL_CLIENTCERT_PEM) );

    /* set MQTT endpoint parameters */
    endpoint_params.transport = AWS_TRANSPORT_MQTT_NATIVE;
    endpoint_params.uri = (char *)AWSIOT_ENDPOINT_ADDRESS;
    endpoint_params.port = AWS_IOT_SECURE_PORT;
    endpoint_params.root_ca = SSL_CA_PEM;
    endpoint_params.root_ca_length = strlen(SSL_CA_PEM);

    /* set MQTT connection parameters */
    conn_params.username = NULL;
    conn_params.password = NULL;
    conn_params.keep_alive = AWSIOT_KEEPALIVE_TIMEOUT;
    conn_params.peer_cn = (uint8_t*)AWSIOT_ENDPOINT_ADDRESS;


    conn_params.client_id = (uint8_t*)AWSIOT_THING_NAME;



    /* connect to an AWS endpoint */
    result = client->connect( conn_params, endpoint_params );
    if ( result != CY_RSLT_SUCCESS )
    {
    	lcd_msg("connection AWS failed %d",0,result);
        APP_INFO(( "connection to AWS endpoint failed\r\n" ));
        if( client != NULL )
        {
            delete client;
            client = NULL;
        }
        return ;
    }

    lcd_msg("connected to AWS: %s",0,AWSIOT_THING_NAME);
    APP_INFO(( "Connected to AWS endpoint\r\n" ));



    //Subscribe after connected

    if(!retry){
    	awsiot_subscribe_shadow("/get/accepted",awsiot_subscribe_get_callback);
    }

	awsiot_subscribe_shadow("/update/accepted",awsiot_subscribe_update_callback);






   // awiot_queue.call_every(AWSIOT_SEND_PERIOD_MS, awsiot_send);


#ifdef SEND_SHADOW
if (!retry)
    awsiot_shadow_cycle();

#endif






    return ;
}

void awsiot_connect_start_thread( NetworkInterface* network){

	awsiot_connect_thread(network,false);

	return;
}

int awsiot_connect( NetworkInterface* network ){

	awsiot_thread.start(callback(&awsiot_connect_start_thread,network));

}




int awsiot_subscribe(const char* topic,subscriber_callback callback){
    cy_rslt_t result = CY_RSLT_SUCCESS;


    	if (client==NULL)
    		return 1;


       result = client->subscribe(topic, AWS_QOS_ATMOST_ONCE, callback );
       if ( result != CY_RSLT_SUCCESS )
       {
    	   lcd_msg("subscribe to AWS failed %d ",2,result);
           APP_INFO(( "subscribe to topic failed\r\n" ));




           return 1;
       }






       APP_INFO(( "subscribe to %s \n",topic ));



       return 0;


}

int awsiot_unsubscribe(const char* topic){
	cy_rslt_t result = client->unsubscribe((char*)topic);


    if ( result != CY_RSLT_SUCCESS )
    {
 	   lcd_msg("unsubscribe to AWS failed %d ",2,result);
        APP_INFO(( "unsubscribe to topic failed\r\n" ));




        return 1;
    }






    APP_INFO(( "unsubscribe to %s \n",topic ));



    return 0;

}


int awsiot_unsubscribe_shadow(char* command){
	char* temp = (char*)malloc(12+7+strlen(command)+strlen(AWSIOT_THING_NAME));
		temp[0]='\0';

		strcat(temp,"$aws/things/");
		strcat(temp,AWSIOT_THING_NAME);
		strcat(temp,"/shadow");
		strcat(temp,command);

		puts(temp);

		command = temp;

		awsiot_unsubscribe(temp);

		free(temp);

}


int awsiot_subscribe_shadow(char* command,subscriber_callback callback){

	char* temp = (char*)malloc(12+7+strlen(command)+strlen(AWSIOT_THING_NAME));
	temp[0]='\0';

	strcat(temp,"$aws/things/");
	strcat(temp,AWSIOT_THING_NAME);
	strcat(temp,"/shadow");
	strcat(temp,command);

	puts(temp);

	command = temp;


	APP_INFO(( "subscribe  %s \n",command ));

		return awsiot_subscribe(command,callback);

}



int awsiot_publish( const char* message,const char* topic){
    cy_rslt_t result = CY_RSLT_SUCCESS;
    publish_params.QoS = AWS_QOS_ATMOST_ONCE;

    	if (client==NULL)
    		return 1;

       result = client->publish( topic, message, strlen(message), publish_params );
       if ( result != CY_RSLT_SUCCESS )
       {
    	   lcd_msg("publish failed",2);
           APP_INFO(( "publish to topic failed, %d \r\n",result ));
           if (_network->get_connection_status() != NSAPI_STATUS_GLOBAL_UP){

        	   _network->disconnect();
        	   _network = NULL;
           }

           awsiot_connect_thread(_network,true);

           return -1;


       }



       lcd_msg_delay_close("published to AWS",2,1000);
       APP_INFO(( "Published to %s - %s",topic,message ));

       return 1;




}

int awsiot_publish( const char* message){
	return awsiot_publish(message,AWSIOT_TOPIC);
}

int awsiot_publish_shadow( const char* message,const char* command){

	char* temp =  (char*)malloc(12+7+strlen(command)+strlen(AWSIOT_THING_NAME));
	temp[0]='\0';
	strcat(temp,"$aws/things/");
	strcat(temp,AWSIOT_THING_NAME);
	strcat(temp,"/shadow");
	strcat(temp,command);




	int result = awsiot_publish(message,temp);

	free(temp);

	if (result!=1)
	{
		APP_INFO(( "Published shadow failure retry\n" ));



		return result;
	}

	APP_INFO(( "Published to shadow %s\n. topic: %s\n",message,temp ));
	lcd_msg_delay_close("AWS shadow published",2,1000);



	return result;
}




void awsiot_update(string name, string value,bool publish){


	if (!publish)
		return;

	awsiot_sem.try_acquire();

	/* store update data under shadow_status_map for waiting published to AWS*/

#if SHADOW_USE_JSON_MAP

	if (!shadow_status_map["state"].value.m){
		//struct JSON state;
		shadow_status_map["state"].value.m = new map<string,JSON>;
		shadow_status_map["state"].type = AWS_JSON_CHILD;

	}



	if (!(*(shadow_status_map["state"].value.m))["reported"].value.m){

		(*(shadow_status_map["state"].value.m))["reported"].value.m = new map<string,JSON>;
		(*(shadow_status_map["state"].value.m))["reported"].type = AWS_JSON_CHILD;

		}


	 (*(*(shadow_status_map["state"].value.m))["reported"].value.m)[name].type = AWS_JSON_STRING;
	 (*(*(shadow_status_map["state"].value.m))["reported"].value.m)[name].value.s = new string(value);
#else

	 if (!shadow_status.empty())
		 shadow_status.append(",");

	 shadow_status.append("\"");
	 shadow_status.append(name);
	 shadow_status.append("\":\"");
	 shadow_status.append(value);
	 shadow_status.append("\"");



#endif

	awsiot_sem.release();

	APP_INFO(( "Waiting Published to shadow %s. hw: %s\n",value.c_str(),name.c_str() ));


}


void awsiot_update(string name, string value){
	awsiot_update(name,value,true);
}



void awsiot_send(){

/*

	if (result != CY_RSLT_SUCCESS){

		if ( result == CY_RSLT_AWS_ERROR_DISCONNECTED )
			awsiot_connect_thread(get_network_interface());

		return;
	}*/


	awsiot_sem.acquire();


#ifdef SEND_SHADOW



#else


	 string sendmessage = "{";

	 	 for (std::map<string,string>::iterator it=awsiot_status_map.begin(); it!=awsiot_status_map.end(); ++it){

	 		 if (it!=awsiot_status_map.begin())
	 			 sendmessage.append(",");

	 	 	sendmessage.append("\"");
	 	 	sendmessage.append(it->first);
	 	 	sendmessage.append("\":\"");
	 	 	sendmessage.append(it->second);
	 	 	sendmessage.append("\"");


	 	 }

	 	 sendmessage.append("}");


	 awsiot_publish(sendmessage.c_str());
	 awsiot_status_map.clear();
	 printf("published and reset map\n");
#endif




	 awsiot_sem.release();

}

