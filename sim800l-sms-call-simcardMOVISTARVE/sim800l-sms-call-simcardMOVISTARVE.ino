/*
  This code reads any SMS in order to know its format which may be
  +CMT: "+584168262667","","17/03/14,16:18:53-16" or
  +CMT: "04161587896","","17/03/14,16:41:41-16"
  It also determines the first and second comma position on the whole string
*/





//--------------------------------Start-Libraries Section----------------------------------------------------//

#include <gprs.h>
#include <SoftwareSerial.h>

//--------------------------------End-Libraries Section----------------------------------------------------//


//--------------------------------Start-Definition Section--------------------------------------------------//

#define TIMEOUT    30000
#define LED_PIN    13
#define TIMEOUTINTERNET 30000

//--------------------------------End-Definition Section--------------------------------------------------//


//--------------------------------Start-Object Creation--------------------------------------------------//

GPRS gprs;

//--------------------------------End-Object Creation--------------------------------------------------//


//--------------------------------Start-Variable Declaration-------------------------------------------//

//Boolean to be set to false if number is on contact
bool ledStatus = HIGH;

////Variable to hold last line of serial output from SIM800
char currentLine[500] = "";
int currentLineIndex = 0;

//Boolean to be set to true if number is found on contact
bool isIncontact = false;

// Variable to hold contact from call
char contact[13]; // changes this if needed (a contact name with more than 13 characters)

// Variable to hold number from call
char phonenumber[13];

// Variable to send SMS
char *number = NULL;

////Boolean to be set to true if call notificaion was found and next line is NOT empty
//bool nextLineIsCall = false;
bool nextValidLineIsCall = false;

////Boolean to be set to true if message notificaion was found and next
////line of serial output is the actual SMS message content
bool nextLineIsMessage = false;

//Boolean to be set to true if number is found on phonebook
bool isInPhonebook = false;

// String which holds the last line read from Serial activitiy
String lastLine = "";

// Integer indexes
int firstComma = -1;
int secondComma = -1;
int thirdComma = -1;
int forthComma = -1;
int fifthComma = -1;
int firstQuote = -1;
int secondQuote = -1;
int len = -1;
int j = -1;
int i = -1;
int f = -1;
int r = -1;

// Contact Counters
int OldCounter = 0;
int NewCounter = 0;

// Temporal variable 
String tmp = "";

// To be saved phonemumber who sent SMS
String phonenum = "";


//--------------------------------End-Variable Declaration-------------------------------------------//



//------------------------------End-Libraries Section--------------------------------------------------------//


//--------------------------------Start-Setup Section-----------------------------------------------//

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial);

  // initialize digital pin 13 as an output.
  pinMode(LED_PIN, OUTPUT);

  //Write current status to LED pin
  digitalWrite(LED_PIN, ledStatus);

  Serial.println("Starting SIM800 SMS-Call Command Processor");
  gprs.preInit();
  delay(1000);

  while (0 != gprs.init())
  {
    delay(1000);
    Serial.print("init error\r\n");
  }

  //Check Call Availability
  if (0 != gprs.sendCmdAndWaitForResp("AT+CCALR?\r\n", "1", TIMEOUT))
  {
    ERROR("ERROR:CCALR");
    return;
  }


  //Set call notification
  if (0 != gprs.sendCmdAndWaitForResp("AT+CLIP=1\r\n", "OK", TIMEOUT))
  {
    ERROR("ERROR:CLIP");
    return;
  }

  //Set SMS mode to ASCII
  if (0 != gprs.sendCmdAndWaitForResp("AT+CMGF=1\r\n", "OK", TIMEOUT))
  {
    ERROR("ERROR:CMGF");
    return;
  }

  //Start listening to New SMS Message Indications
  if (0 != gprs.sendCmdAndWaitForResp("AT+CNMI=1,2,0,0,0\r\n", "OK", TIMEOUT))
  {
    ERROR("ERROR:CNMI");
    return;
  }

  Serial.println("Init success");
}

//--------------------------------End-Setup Section----------------------------------------------//


//--------------------------------Start loop Section----------------------------------------------//
void loop()
{
  //Write current status to LED pin
  digitalWrite(LED_PIN, ledStatus);

  //If there is serial output from SIM800
  if (gprs.serialSIM800.available())
  {
    char lastCharRead = gprs.serialSIM800.read();

    //Read each character from serial output until \r or \n is reached (which denotes end of line)
    if (lastCharRead == '\r' || lastCharRead == '\n')
    {
      endoflinereached();
    }

    else
    {
      currentLine[currentLineIndex++] = lastCharRead;
    }
  }
}

//--------------------------------End-loop Section----------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////////////////
void endoflinereached()
{
  lastLine = String(currentLine);

  //-----------------------Call---------------------------//
  // If lastLine reads RING, New Call Indication was received.
  // Hence, the THIRD LINE is the caller information.

  // The whole string will be something like this if caller IS registered on SIM Card:
  //RING (First Line)
  //     (Second Line is empty)
  //+CLIP: "04168262667",129,"",0,"Yoimer",0 (Third Line)

  // The whole string will be something like this if caller is NOT registered on SIM Card
  //RING (First Line)
  //     (Second Line is empty)
  //+CLIP: "04168262667",129,"",0,"",0 (Third Line)

  //----------------------SMS-------------------------------------//
  //If lastLine reads +CMT, New SMS Message Indications was received.
  //Hence, next line is the message content.

  if (lastLine.startsWith("RING"))                                   // New incoming call
  {
    Serial.println(lastLine);
    nextValidLineIsCall = true;
  }
  else if ((lastLine.length() > 0) && (nextValidLineIsCall))        // Rejects any empty line
  {
    LastLineIsCLIP();
  }
  else if (lastLine.startsWith("+CMT:"))                           // New incoming SMS
  {
    Serial.println(lastLine);
    phonenum = lastLine.substring((lastLine.indexOf(34) + 1),   //Parser to extract phonenumber
               lastLine.indexOf(34, lastLine.indexOf(34) + 1));
    ////Serial.println(phonenum);
    nextLineIsMessage = true;

    // Parsing lastLine to determine registration on SIM card
    firstComma = lastLine.indexOf(',');
    Serial.println(firstComma);  //For debugging
    secondComma = lastLine.indexOf(',', firstComma + 1);
    Serial.println(secondComma); //For debugging

    // +CMT: "+584168262667","","17/03/14,16:18:53-16" TelefÃ³nica Movistar Venezuela with no contacts saved
    //firstComma = 21
    //secondComma = 24

    Serial.println("Diff");
    Serial.println(secondComma - firstComma);
    

  }
  else if ((lastLine.length() > 0) && (nextLineIsMessage))       // Rejects any empty line
  {

    ////LastLineIsCMT();
    LastLineIsCMT();

  }

  CleanCurrentLine();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CleanCurrentLine()
{
  //Clear char array for next line of read
  for ( int i = 0; i < sizeof(currentLine);  ++i )
  {
    currentLine[i] = (char)0;
  }
  currentLineIndex = 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////
void LastLineIsCMT()
{
  if (nextLineIsMessage)
  {
    Serial.println(lastLine);


    // If exists on Phonebook
    //if (secondComma > 22)   // Only works with Movilnet and Digitel Venezuela
    //if (secondComma > 24)    // Only works with for TelefÃ³nica Movistar Venezuela
    if ((secondComma - firstComma) > 3)
    {
      Serial.println("In Phonebook"); //For debugging
      isInPhonebook = true;
      Serial.println(isInPhonebook);
    }
    else
    {
      Serial.println("Not in Phonebook"); //For debugging
      isInPhonebook = false;
    }

    //if on phonebook ---------------------------
    if (isInPhonebook)
    {
      // If SMS contains LED ON or LED OFF or #WhiteList
      if (lastLine.indexOf("LED ON") >= 0)
      {
        ledStatus = 1;   // Turns ON LED
      }
      else if (lastLine.indexOf("LED OFF") >= 0)
      {
        ledStatus = 0;  // Turns OFF LED
      }
      else if (lastLine.indexOf("ADD") >= 0)
      {
        //Serial.println("Go to ADD routine");
        AddContact();
      }
      else if (lastLine.indexOf("DEL") >= 0)
      {
        //Serial.println("Go to DEL routine");
        DelContact();
      }
      else if (lastLine.indexOf("N@P") >= 0)
      {
        //Serial.println("Go to NumberAtPos routine");
        NumberAtPosition();
      }
    }
    CleanCurrentLine();
    nextLineIsMessage = false;
  }
}
//////////////////////////////////////////////////////////////////////////
void LastLineIsCLIP()
{
  if (nextValidLineIsCall)
  {
    Serial.println(lastLine);

    // Parsing lastLine to determine registration on SIM card
    firstComma = lastLine.indexOf(',');
    Serial.println(firstComma);  //For debugging
    secondComma = lastLine.indexOf(',', firstComma + 1);
    Serial.println(secondComma); //For debugging
    thirdComma = lastLine.indexOf(',', secondComma + 1);
    Serial.println(thirdComma);  //For debugging
    forthComma = lastLine.indexOf(',', thirdComma + 1);
    Serial.println(forthComma); //For debugging
    fifthComma = lastLine.indexOf(',', forthComma + 1);
    Serial.println(fifthComma); //For debugging

    //Extracts contact
    j = 0;
    for (int i = forthComma + 1; i < fifthComma; ++i) {
      contact[j] = lastLine[i];
      ++j;
    }
    contact[j] = '\0'; // Contact as a full string
    Serial.println(contact); //For Debugging

    len = strlen(contact); //lenght of contact string
    Serial.println(len);  // For Debugging

    // HERE GOES the extraction of mobile number code

    ////ExtractPhoneNumber();
    //Serial.println(number);

    // If exists on contact
    if (len > 2)
    {
      Serial.println("In contact"); //For debugging
      isIncontact = true;
      Serial.println(isIncontact);
    }
    else
    {
      Serial.println("Not in contact"); //For debugging
      isIncontact = false;
    }

    // If registered turns off led on pin 13.
    //If not, just do nothing. (In a later release the action of turning off the led will notify the caller via SMS)

    if (isIncontact)
    {
      ledStatus = 0;
      digitalWrite(LED_PIN, ledStatus);
    }
    ////CleanContactArray();
    nextValidLineIsCall = false;
  }
}
/////////////////////////////////////////////////////////////////////////////////////////
void AddContact()
{
  /////String indexAndName = lastLine.substring(4, 5);
  firstComma = lastLine.indexOf(',');
  secondComma = lastLine.indexOf(',', firstComma + 1);
  thirdComma = lastLine.indexOf(',', secondComma + 1);
  String indexAndName = lastLine.substring((firstComma + 1), (secondComma)); //Position and name to be saved on SIM
  //Serial.println(indexAndName);
  String newContact = lastLine.substring((secondComma + 1), thirdComma);  // Number to be saved on SIM
  //Serial.println(newContact);
  tmp = ""; // Cleans tmp
  tmp = "AT+CPBW=" + indexAndName + ",\"" + newContact + "\"" + ",129," + "\"" + indexAndName + "\"" + "\r\n\"";
  Serial.println(tmp);
  if (0 != gprs.sendCmdAndWaitForResp(tmp.c_str(), "OK", TIMEOUT))
  {
    //    //ERROR("ERROR:CPBW");
    Serial.println("No added");
  }
  else
  {
    Serial.println("Added");
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////
void DelContact()
{
  //String indexAndName = lastLine.substring(4, 5);   // Position and name to be deleted on SIM
  /////String indexAndName = lastLine.substring(4, 5);
  firstComma = lastLine.indexOf(',');
  secondComma = lastLine.indexOf(',', firstComma + 1);
  thirdComma = lastLine.indexOf(',', secondComma + 1);
  String indexAndName = lastLine.substring((firstComma + 1), (secondComma)); //Position and name to be saved on SIM
  //Serial.println(indexAndName);
  String newContact = lastLine.substring((secondComma + 1), thirdComma);  // Number to be saved on SIM
  //Serial.println(newContact);
  tmp = ""; // Cleans tmp
  tmp = "AT+CPBW=" + indexAndName + "\r\n\"";
  Serial.println(tmp);
  if (0 != gprs.sendCmdAndWaitForResp(tmp.c_str(), "OK", TIMEOUT))
  {
    //ERROR("ERROR:CPBW");
    Serial.println("No deleted.");
  }
  else
  {
    Serial.println("Deleted");
  }
}
/////////////////////////////////////////////////////////////////////////////////////////
void NumberAtPosition()
{
  firstComma = lastLine.indexOf(',');
  secondComma = lastLine.indexOf(',', firstComma + 1);
  String indexAndName = lastLine.substring((firstComma + 1), (secondComma)); //Position to be asked
  //Serial.println(indexAndName);
  tmp = ""; // Cleans tmp
  tmp = "AT+CPBR=" + indexAndName + "," + indexAndName + "\r\n\"";
  //tmp = "AT+CPBR=1," + indexAndName + "\r\n\"";
  //AT+CPBR=1,250

  ////Serial.println(tmp);
  if (0 != gprs.sendCmdAndWaitForResp(tmp.c_str(), "+CPBR:", TIMEOUT))
  {
    Serial.println("Not Good");
  }
  else
  {
    Serial.println("Good");
//    CleanCurrentLine();
//    lastLine = "";
//    currentLineIndex = 0;
    while (gprs.serialSIM800.available())
    {
      gprs.serialSIM800.read();
    }
    sendATcommand(tmp.c_str(), "OK\r\n", TIMEOUT);
  }
}
////////////////////////////////////////////////////////
int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout)
{

  uint8_t x = 0,  answer = 0;
  char response[100];
  unsigned long previous;

  memset(response, '\0', 100);    // Initialice the string

  delay(100);

  while (gprs.serialSIM800.available())
  {
    gprs.serialSIM800.read();
  }

  Serial.println(ATcommand);    // Prints the AT command
  gprs.sendCmd(ATcommand);


  x = 0;
  previous = millis();

  // this loop waits for the answer
  do
  {
    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (gprs.serialSIM800.available() != 0)
    {
      response[x] = gprs.serialSIM800.read();
      x++;
      // check if the desired answer is in the response of the module
      if (strstr(response, expected_answer) != NULL)
      {
        answer = 1;
        //Serial.println(response);
        String numbFromSim = String(response);
        
        numbFromSim = numbFromSim.substring(numbFromSim.indexOf(":"), 
                      numbFromSim.indexOf("129")); //First parser to extract phonenumber
                      
        numbFromSim = numbFromSim.substring((numbFromSim.indexOf(34) + 1), 
                      numbFromSim.indexOf(34, numbFromSim.indexOf(34) + 1)); //Final parser to extract phonenumber
     
        char SMS[12]; // 11 chars for phonumber plus null character
        char phonenumber[14]; // NOT ANYLONGER 14 --> 13 chars (including +58) plus null character (ONLY Movistar Venezuela)
                              // NOT ANYLONGER 12 --> 11 chars plus null character (ONLY Movilnet and Digitel Venezuela)
                              // 14 ----> for any phonenumber that has 11 digits plus sign char and two digits as
                              // international code number +5804168262669 for example)
                             
        numbFromSim.toCharArray(SMS, 12);
        phonenum.toCharArray(phonenumber, 14);
        gprs.sendSMS(phonenumber, SMS);
      }
    } // Waits for the asnwer with time out
  } while ((answer == 0) && ((millis() - previous) < timeout));
}

