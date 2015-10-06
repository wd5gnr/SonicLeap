// Sonic poor man's Leap Controller
#define USE_NEW_PING 0
#if USE_NEW_PING==1
#include <NewPing.h>
#endif

// Tuning parameters
#if USE_NEW_PING==1
#define AVG 16
#define DELTATHRESHOLD 300   // changes have to exceed this amount to register
#define IDLETHRESHOLD 8000   // any duration >=this is 
#else
// The raw data is very noisy, so we are going to average
#define AVG 32  // set # of samples
#define DELTATHRESHOLD 350   // changes have to exceed this amount to register
#define IDLETHRESHOLD 3500   // any duration >=this is 
#endif



// Connections to HCS04
const int pingPin = 3;
const int echoPin = 8;

#if USE_NEW_PING==1
NewPing sonar(pingPin,echoPin);
#endif

void setup() {
#if USE_NEW_PING==1
#else
  pinMode(pingPin,OUTPUT);
  pinMode(echoPin,INPUT);
#endif
  // initialize serial communication:
  Serial.begin(9600);
}

// Work around possible bad abs macro and sign issues
unsigned xabs(unsigned a, unsigned  b)
{
  unsigned diff;
  diff=a<b?b-a:a-b;
  return diff;
}

#if USE_NEW_PING==1
unsigned get_distance(void)
{
  return sonar.ping_median(AVG);
}

#else

// Get raw distance
unsigned get_rawdistance(void)
{
  unsigned  duration;
  delay(10);
  // Trigger the Ranging device
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);   // 10us minimum pulse
  digitalWrite(pingPin, LOW);
  duration = pulseIn(echoPin, HIGH);   // read echo
  return duration;
}


unsigned  get_distance(void)
{
  unsigned long duration=0;  
  unsigned last=-1;
  for (int i=0;i<AVG;) // average over this many samples (note no i++ here)
    {
      unsigned d=get_rawdistance();
      unsigned delta=xabs(last,d);  // find the delta
      last=d;   // remember this value
      if (last>=0 && delta>3*DELTATHRESHOLD) continue;  // filter out wild values
      i++;  // good value so count it for our average
      duration+=d;  // add to running total
    }
  duration/=AVG;   // find average
  return (unsigned)duration;
}

#endif

// Here's where you can do your actions
// Action states
#define START_IN 0
#define STOP_IN 1
#define UP_IN 2
#define DN_IN 3

void action(int why, unsigned value=0)
{
  Serial.print(value);
  switch (why)
  {
    case START_IN:
      Serial.println(" Start");
    break;
    case STOP_IN:
      Serial.println(" Stop");
    break;
    case UP_IN:
      Serial.println(" Up");
      Remote.increase();
    break;
    case DN_IN:
      Serial.println(" Down");
      Remote.decrease();
    break;
  }
}


// States
#define IDLE 0  // no input
#define SAMP 1  // sampling input


// Current state
int state=IDLE;

void loop()
{
   unsigned prev;  

   while (1)
   {

     unsigned dur=get_distance();  // get averaged distance
#if 0 // enable if you just want to dump values
     Serial.println(dur);
     delay(500);
     continue;
#endif
     
     switch (state)
     {
       case IDLE:  // idle state; see if we see anything yet?
         if (dur>=IDLETHRESHOLD || dur==0) continue; // no change
       // fall through if <IDLETHRESHOLD
       case SAMP:   // Sampling
         if (dur>=IDLETHRESHOLD || dur==0)   // check to see if done
         {
           action(STOP_IN,dur);
           state=IDLE;
           continue;
         }
         if (state==IDLE) // remember, fell through above so could be idle or samp
         {
           // make sure it is real
           delay(100);
           dur=get_distance();
           if (dur>=IDLETHRESHOLD) continue; // not real, just a blip
           prev=dur;
           state=SAMP;
           action(START_IN,dur);
           continue;
         }
        else
        {
           int delta=dur>prev?UP_IN:DN_IN;   // speculate on up or down
           if (xabs(dur,prev)<DELTATHRESHOLD) continue; // no change (not enough up or down)
           prev=dur;   // remember for next time
           // Something happened, ask user what to do
           action(delta,prev);
           delay(50); 
           continue;
        }
       
       break;
       
     }
   }



}



