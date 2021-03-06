.. _park_api:


.. _park_parkedcalls:

/park/parkedcalls
=================

Methods
-------
GET : Get list of all parked_calls info.

.. _get_park_parkedcalls:

Method: GET
-----------
Get list of all parked_calls info.

Call
++++
::

   GET /park/parkedcalls

Returns
+++++++
::

   {
     $defhdr,
     "reuslt": {
       "list": [
         {
           ...
         },
         ...
       ]
     }
   }

Return parameters

* ``list`` : array of itmes.
    * See detail parkedcall detail info.

Example
+++++++
::

  $ curl -X GET localhost:8081/park/parkedcalls
  
  {
    "api_ver": "0.1",
    "result": {
        "list": [
            {
                "parkee_account_code": "",
                "parkee_caller_id_name": "pjagent-01",
                "parkee_caller_id_num": "pjagent-01",
                "parkee_channel": "PJSIP/pjagent-01-00000044",
                "parkee_channel_state": "6",
                "parkee_channel_state_desc": "Up",
                "parkee_connected_line_name": "<unknown>",
                "parkee_connected_line_num": "<unknown>",
                "parkee_context": "sample_park",
                "parkee_exten": "s",
                "parkee_linked_id": "1514423807.111",
                "parkee_priority": "2",
                "parkee_unique_id": "1514423807.111",
                "parker_dial_string": "PJSIP/pjagent-01",
                "parking_duration": 0,
                "parking_lot": "default",
                "parking_space": "701",
                "parking_timeout": 45,
                "tm_update": "2017-12-28T01:16:48.343347741Z"
            }
        ]
    },
    "statuscode": 200,
    "timestamp": "2017-12-28T01:17:10.187476889Z"
  }


.. _park_parkedcalls_detail:
  
/park/parkedcalls/<detail>
==========================

Methods
-------
GET : Get parked_call's detail info of given info.

DELETE : Hangup the given parked call info.

.. _get_park_parkedcalls_detail:

Method: GET
-----------
Get parked_call's detail info of given info.

Call
++++
::

   GET /park/parkedcalls/<detail>

Method parameters

* ``detail``: Parkee's unique id.

Returns
+++++++
::

   {
     $defhdr,
     "reuslt": {
       "parkee_unique_id": "<string>",
       "parkee_linked_id": "<string>",

       "parkee_channel": "<string>",
       "parkee_channel_state": "<string>",
       "parkee_channel_state_desc": "<string>",

       "parkee_caller_id_name": "<string>",
       "parkee_caller_id_num": "<string>",

       "parkee_connected_line_name": "<string>",
       "parkee_connected_line_num": "<string>,

       "parkee_account_code": "<string>",

       "parkee_context": "<string>",
       "parkee_exten": "<string>",
       "parkee_priority": "<string>",

       "parker_dial_string": "<string>",
       "parking_duration": <integer>,
       "parking_lot": "<string>",
       "parking_space": "<string>",
       "parking_timeout": <integer>,
       
       "tm_update": "<timestamp>"
     }
   }

Return parameters

* ``parkee_unique_id``: Parkee's unique id.
* ``parkee_linked_id``: Parkee's linked id.

* ``parkee_channel``: Parkee's channel name.
* ``parkee_channel_state``: Parkee's channele state.
* ``parkee_channel_state_desc``: Parkee's channel state description.

* ``parkee_caller_id_name``: Parkee's caller id name.
* ``parkee_caller_id_num``: Parkee's caller id number.

* ``parkee_connected_line_name``: Parkee's connected line name.
* ``parkee_connected_line_num``: Parkee's connected line number.

* ``parkee_account_code``: Parkee's account code.

* ``parkee_context``: Parkee's current context.
* ``parkee_exten``: Parkee's current exten.
* ``parkee_priority``: Parkee's current priority.

* ``parker_dial_string``: Dial String that can be used to call back the parker on ParkingTimeout.
* ``parking_duration``: Time the parkee has been in the parking bridge since tm_update(in seconds).
* ``parking_lot``: Name of the parking lot that the parkee is parked in.
* ``parking_space``: Parking Space that the parkee is parked in.
* ``parking_timeout``: Time remaining until the parkee is forcefully removed from parking in seconds since tm_update.

Example
+++++++
::

  $ curl -X GET localhost:8081/park/parkedcalls/1514423984.115
  
  {
    "api_ver": "0.1",
    "result": {
        "parkee_account_code": "",
        "parkee_caller_id_name": "pjagent-01",
        "parkee_caller_id_num": "pjagent-01",
        "parkee_channel": "PJSIP/pjagent-01-00000046",
        "parkee_channel_state": "6",
        "parkee_channel_state_desc": "Up",
        "parkee_connected_line_name": "<unknown>",
        "parkee_connected_line_num": "<unknown>",
        "parkee_context": "sample_park",
        "parkee_exten": "s",
        "parkee_linked_id": "1514423984.115",
        "parkee_priority": "2",
        "parkee_unique_id": "1514423984.115",
        "parker_dial_string": "PJSIP/pjagent-01",
        "parking_duration": 0,
        "parking_lot": "default",
        "parking_space": "701",
        "parking_timeout": 45,
        "tm_update": "2017-12-28T01:19:44.271005802Z"
    },
    "statuscode": 200,
    "timestamp": "2017-12-28T01:19:53.629685348Z"
  }


.. _delete_park_parkedcalls_detail:

Method: DELETE
--------------
Hangup the given parked call info.

Call
++++
::

   DELETE /park/parkedcalls/<detail>

Method parameters

* ``detail``: Parkee's unique id.

Returns
+++++++
::

   {
     $defhdr
   }

Example
+++++++
::

  $ curl -X DELETE localhost:8081/park/parkedcalls/1515019232.8
  
  {
    "api_ver": "0.1",
    "statuscode": 200,
    "timestamp": "2018-01-03T22:40:50.55634319Z"
  }


.. _park_parkinglots:

/park/parkinglots
=================

Methods
-------
GET : Get list of all parking lot info.

POST : Create new parking lot info.

.. _get_park_parkinglots:

Method: GET
-----------
Get list of all parking lots info.

Call
++++
::

   GET /park/parkinglots

Returns
+++++++
::

   {
     $defhdr,
     "reuslt": {
       "list": [
         {
           ...
         },
         ...
       ]
     }
   }

Return parameters

* ``list`` : array of itmes.
   * See detail at parking lot detail info.

Example
+++++++
::

  $ curl -X GET localhost:8081/park/parkinglots
  
  {
    "api_ver": "0.1",
    "result": {
        "list": [
            {
                "name": "default",
                "start_space": "701",
                "stop_spcae": "720",
                "timeout": 45,
                "tm_update": "2017-12-28T01:16:46.350523532Z"
            }
        ]
    },
    "statuscode": 200,
    "timestamp": "2017-12-28T01:21:57.180448453Z"
  }


.. _post_park_parkinglots:

Method: POST
------------
Create new parking lot info.

Call
++++
::

  POST /park/parkinglots
   
  {
    ...
  }

Returns
+++++++
::

  {
    $defhdr
  }


Example
+++++++
::

  $ curl -X POST localhost:8081/park/parkinglots -d 
  '{"name": "test_parkinglot", "context": "> edvina_park", \
  "parkpos": "> 800-850", "findslot": "> next", \
  "comebacktoorigin": "no", "comebackdialtime": "90", \
  "comebackcontext": "edvinapark-timeout", "parkedmusicclass": "edvina" }'
  
  {
    "api_ver": "0.1",
    "timestamp": "2018-01-03T21:12:39.553826772Z",
    "statuscode": 200
  }
  

.. _park_parkinglots_detail:
  
/park/parkinglots/<detail>
==========================

Methods
-------
GET : Get parking lot's detail info of given detail.

PUT : Update parking lot's detail info of given detail.

DELETE : Delete the given parking lot info.

.. _get_park_parkinglots_detail:

Method: GET
-----------
Get parking lot's detail info of given info.

Call
++++
::

  GET /park/parkinglots/<detail>


Method parameters

* ``detail``: url encoded parking lot's name.

Returns
+++++++
::

  {
    $defhdr,
    "reuslt": {
      "name": "<string>",

      "start_space": "<string>",
      "stop_spcae": "<string>",

      "timeout": <integer>,
      
      "tm_update": "<timestamp>"
    }
  }

Return parameters

* ``name``: Parking lot's name.

* ``start_space``: Parking lot's start space.
* ``stop_spcae``: Parking lot's stop space.

* ``timeout``: Timeout second in the parking lot.

Example
+++++++
::

  $ curl -X GET localhost:8081/park/parkinglots/default
  
  {
    "api_ver": "0.1",
    "result": {
        "name": "default",
        "start_space": "701",
        "stop_spcae": "720",
        "timeout": 45,
        "tm_update": "2017-12-28T01:16:46.350523532Z"
    },
    "statuscode": 200,
    "timestamp": "2017-12-28T01:25:47.123913131Z"
  }



.. _put_park_parkinglots_detail:

Method: PUT
-----------
Update parking lot's detail info of given detail.

Call
++++
::

  PUT /park/parkinglots/<detail>
  
  {
    ...
  }


Method parameters

* ``detail``: url encoded parking lot's name.

Returns
+++++++
::

  {
    $defhdr
  }

Example
+++++++
::

  $ curl -X PUT localhost:8081/park/parkinglots/test_parkinglot -d \
  '{"context": "> edvina_park", "parkpos": "> 800-900", "findslot": "> next", \
  "comebacktoorigin": "no", "comebackdialtime": "90", \
  "comebackcontext": "edvinapark-timeout", "parkedmusicclass": "edvina" }'
  
  {
    "api_ver": "0.1",
    "timestamp": "2018-01-03T22:17:22.76376966Z",
    "statuscode": 200
  }

  
  
.. _delete_park_parkinglots_detail:

Method: DELETE
--------------
DELETE : Delete the given parking lot info.

Call
++++
::

  DELETE /park/parkinglots/<detail>

Returns
+++++++
::

  {
    $defhdr
  }

Example
+++++++
::

  $ curl -X DELETE localhost:8081/park/parkinglots/test_parkinglot
  
  {
    "api_ver": "0.1",
    "timestamp": "2018-01-03T21:50:01.672074605Z",
    "statuscode": 200
  }

  
  
/park/setting
==============

Methods
-------
GET : Get current park setting.

PUT : Update park setting.

.. _get_park_setting:

Method: GET
-----------
GET : Get current park setting.

This result does not mean to currently running setting. 
It shows only setting file.

Call
++++
::

  GET ^/park/setting
  
Returns
+++++++
::

  {
    $defhdr,
    "result": {
      ...
    }
  }

Return parameters

* See detail at park setting.

Example
+++++++
::

  $ curl -X GET localhost:8081/park/setting
  
  {
    "api_ver": "0.1",
    "result": {
        "default": {
            "context": "> parkedcalls",
            "parkext": "> 700",
            "parkpos": "> 701-720"
        },
        "general": {},
        "test_parkinglot": {
            "comebackcontext": "edvinapark-timeout",
            "comebackdialtime": "90",
            "comebacktoorigin": "no",
            "context": "> edvina_park",
            "findslot": "> next",
            "parkedmusicclass": "edvina",
            "parkpos": "> 800-900"
        }
    },
    "statuscode": 200,
    "timestamp": "2018-01-03T22:51:19.20572144Z"
  }


.. _put_park_setting:

Method: PUT
-----------
PUT : Update park setting.

Update only setting file. To adapt to module, required module reload.

Call
++++
::

  PUT ^/park/setting
  
  {
    ...
  }

Data parameters

* park setting info.
  
Returns
+++++++
::

  {
    $defhdr
  }
   
Example
+++++++
::

  $ curl -X PUT localhost:8081/park/setting -d 
  '
  {
    "default": {
      "context": "> parkedcalls",
      "parkext": "> 700",
      "parkpos": "> 701-720"
    },
    "general": {},
    "test_parkinglot": {
      "comebackcontext": "edvinapark-timeout",
      "comebackdialtime": "90",
      "comebacktoorigin": "no",
      "context": "> edvina_park",
      "findslot": "> next",
      "parkedmusicclass": "edvina",
      "parkpos": "> 800-1000"
    }
  }
  '
  
  {"api_ver": "0.1", "timestamp": "2018-01-03T22:59:02.195183096Z", "statuscode": 200}
  

.. _park_settings:

/park/settings
==============

Methods
-------
GET : Get all backup park settings.

.. _get_park_settings:

Method: GET
-----------
GET : Get all backup park settings.


Call
++++
::

  GET ^/park/settings
  
Returns
+++++++
::

  {
    $defhdr,
    "reuslt": {
      "list": [
        {
          "filename": "<string>",
          ...
        }
      ]
    }
  }

Return parameters

* ``list``: array of backup files.
    * ``filename``: backup filename.
   
Example
+++++++
::

  $ curl -X GET localhost:8081/park/settings
  
  {
    "api_ver": "0.1",
    "result": {
        "list": [
            {
                "default": {
                    "context": "> parkedcalls",
                    "parkext": "> 700",
                    "parkpos": "> 701-720"
                },
                "filename": "res_parking.conf.2018-01-03T21:12:39.537427535Z",
                "general": {}
            },
            ...
        ]
    },
    "statuscode": 200,
    "timestamp": "2018-01-03T23:07:58.415769305Z"
  }

  
.. _park_settings_detail:

/park/settings/<detail>
========================

Methods
-------
GET : Get specified backup park settings.

DELETE : Delete given backup park setting.

.. _get_park_settings_detail:

Method: GET
-----------
GET : Get backup park settings of given info.


Call
++++
::

  GET ^/park/settings/<detail>
  
Method parameters

* ``detail``: backup filename.
  
Returns
+++++++
::

  {
    $defhdr,
    "reuslt": {
      ...
    }
  }

* See detail park setting.
   
Example
+++++++
::

  $ curl -X GET localhost:8081/park/settings/res_parking.conf.2018-01-03T22:59:02.177676713Z
  
  {
    "api_ver": "0.1",
    "result": {
        "list": {
            "default": {
                "context": "> parkedcalls",
                "parkext": "> 700",
                "parkpos": "> 701-720"
            },
            "general": {},
            "test_parkinglot": {
                "comebackcontext": "edvinapark-timeout",
                "comebackdialtime": "90",
                "comebacktoorigin": "no",
                "context": "> edvina_park",
                "findslot": "> next",
                "parkedmusicclass": "edvina",
                "parkpos": "> 800-900"
            }
        }
    },
    "statuscode": 200,
    "timestamp": "2018-01-03T23:26:00.784492928Z"
  }

  
.. _delete_park_settings_detail:
  
Method: DELETE
--------------
DELETE : Delete given backup park setting.


Call
++++
::

  DELETE ^/park/settings/<detail>
  
Method parameters

* ``detail``: backup filename.
  
Returns
+++++++
::

  {
    $defhdr
  }
   
Example
+++++++
::

  $ curl -X DELETE localhost:8081/park/settings/res_parking.conf.2018-01-03T22:59:02.177676713Z
  
  {
    "api_ver": "0.1",
    "statuscode": 200,
    "timestamp": "2018-01-03T23:26:10.361720786Z"
  }
