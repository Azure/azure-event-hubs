/*Copyright (c) Microsoft Corporation.  All rights reserved.

Licensed under the Apache License, Version 2.0 (the ""License"");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER
EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE,
FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.

See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.*/

#include "eventhubclient.h"
#include "eventdata.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
  // These default (fallback) values will be used if not configuration file is provided
  const char *DEFAULT_CONNECTION_STRING = "INSERT CONNECTION_STRING_HERE";
  const char *DEFAULT_EVENTHUB_PATH = "loadavgin";

  const char *connectionString;
  const char *eventHubPath;

  const char* msgFormat = "{\"device\":\"%s\", \"load1\":%f, \"load5\":%f, \"load15\":%f, \"activetasks\":%d, \"totaltasks\":%d, \"pid\":%d}";
  char msgText[512];
  char deviceid[64];
  float load1, load5, load15;
  EVENTHUBCLIENT_RESULT result;

  // Retrieve Event Hub configuration from environment variables
  connectionString = getenv("CONNECTION_STRING");
  if (!connectionString) {
    connectionString = DEFAULT_CONNECTION_STRING;
  }
  eventHubPath = getenv("EVENTHUB_NAME");
  if (!eventHubPath) {
    eventHubPath = DEFAULT_EVENTHUB_PATH;
  }
  printf("connection string: %s\n", connectionString);
  printf("event hub name: %s\n", eventHubPath);

  // Device ID can be passed as parameter

  if (argc < 2) {
    fprintf(stderr, "%s: using hostname for deviceid\n", argv[0]);
    gethostname(deviceid, sizeof(deviceid));
  } else {
    fprintf(stderr, "%s: using deviceid = %s\n", argv[0], argv[1]);
    strcpy(deviceid, argv[1]);
  }

  // Start the Event Hub client

  printf("Starting the EventHubClientSample...\n");

  EVENTHUBCLIENT_HANDLE eventHubClientHandle = EventHubClient_CreateFromConnectionString(connectionString, eventHubPath);

  if (eventHubClientHandle == NULL) {
    printf("ERROR: eventHubClientHandle is NULL!\n");
    exit(1);
  }

  while (1) {
    // Get load averages
    loadavg(&load1, &load5, &load15);

    // Assemble message body
    snprintf(msgText, sizeof(msgText), msgFormat, deviceid, load1, load5, load15, 42, 100, 4200);

    EVENTDATA_HANDLE eventDataHandle = EventData_CreateWithNewMemory(msgText, strlen(msgText));

    if (eventDataHandle == NULL) {
      printf("ERROR: eventDataHandle is NULL!\n");
      break;
    }

    result = EventHubClient_Send(eventHubClientHandle, eventDataHandle);
    if (result != EVENTHUBCLIENT_OK) {
      printf("WARNING: EventHubClient_Send failed: %d\n", result);
      // Don't break (keep running) in case this is a temporary failure
    } else {
      //printf("EventHubClient_Send.......Successful\n%s\n", msgText);
    }

    EventData_Destroy(eventDataHandle);

    sleep(1); // Sleep for one second
  }

  EventHubClient_Destroy(eventHubClientHandle);

  return 1;
}

int loadavg(float *load1, float *load5, float *load15)
{
  FILE*f;
  int n;

  *load1 = 0.0;
  *load5 = 0.0;
  *load15 = 0.0;

  if ((f = fopen("/proc/loadavg", "r")) == NULL) return 0;
  n = fscanf(f, "%f %f %f", load1, load5, load15);
  fclose(f);

  return 1;
}
