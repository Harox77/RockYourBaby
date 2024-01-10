#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <libpynq.h>
#include <pthread.h>

// how much time to wait between loop cycles
#define CYCLE 5
#define MAXIMUM 100

typedef struct{
    float freq; // frequency
    float amp; // amplitude
}matrix_t;


// SEND thread arguments
typedef struct send_args_t {
  uint8_t data; // the data to send
  uart_index_t portTX; // UART peripheral/port which sends
  uart_index_t portRX; // UART peripheral/port which receives
} send_args_t;

// RECEIVE thread arguments
typedef struct rec_args_t {
  uint8_t data; // element carrying received data
  uart_index_t portRX; // UART peripheral/port which receives the data
} rec_args_t;

// Display stuff
display_t display;
FontxFile font[2];
uint8_t fontWidth;
uint8_t fontHeight;
uint8_t buffer_font[1000] = {0,};

// Makes an integer printable on the display
uint8_t* convertIntegerToASCII(uint8_t N)
{

    uint8_t m = N;
    uint8_t digit = 0;

    do{
        digit++;
        m /= 10;
    }while (m);

    uint8_t* arr;
    uint8_t arr1[digit];
 
    arr = (uint8_t*)malloc(digit);

    uint8_t index = 0;
    do{
        arr1[++index] = N % 10 + '0';
        N /= 10;
    }while (N);
    uint8_t i;
    for (i = 0; i < index; i++) {
        arr[i] = arr1[index - i];
    }
 
    arr[i] = '\0';
    return (uint8_t*)arr;
}

// Send asynchronously (test)
void* send(void* arg){
  // get the data and ports responsible for this transmission
  send_args_t* sendArgs = (send_args_t *) arg;

  // while the receiving port does not have data
  //while(!uart_has_data(sendArgs->portRX)){
  // clear queue of transmission queue to send new one
  uart_reset_fifos(sendArgs->portTX);
  // send updated data
  uart_send(sendArgs->portTX, sendArgs->data);
  // wait so that uart_has_data has time to update
  sleep_msec(100);
  //}

  // assign this memory location to the placeholder pointer for the sender data (sentF/sentA) for debugging purposes
  return &sendArgs->data;

}

void* receive(void* arg){
  // get the struct with the receive arguments
  rec_args_t * args = (rec_args_t*)arg;
  // DO NOT RESET BECAUSE 6 MS IS NOT ENOUGH FOR TWO INPUTS TO BE READ AND SENT AGAIN
  // uart_reset_fifos(args->portRX);
  // sleep_msec(6);
  
  // store the data in the data member of the args
  args->data = uart_recv(args->portRX);
  // return the memory location of the args to store it in the placeholder pointer (rcArgsFP)
  return args;
}
        

//function that generates random numbers, used for debugging
void randomNumber(int * frequency, int * amplitude){
  int ran1, ran2; ran1 = 0; ran2 = 0;

  srand(time(NULL));

  ran1 = rand() % 101;
  ran2 = rand() % 101;

  *frequency = ran1;
  *amplitude = ran2;

}

int calculateStressLevel(int heartbeat, int crying){
  //need to calculate it by looking at the graphs
  int stressLevel = 0;
  int stress1 = 0;
  int stress2 = 0;

  //the formulas are the inverse functions of the ones in the documentation, this is so the functions can be calculated for stressLevel and not the other way around


  //first situation, both heartbeat and crying are below 10 and thus stressLevel is 10
  if(heartbeat <= 10 && crying <= 10){
      stressLevel = 10;
  }
  //second situation, crying is over 50 and does not influence stress level anymore
  else if(crying >= 50){
      stressLevel = (100+heartbeat)/2;
  }
  //third situation, heartbeat and crying are both in the ranges that make them both important to the calculation
  else if(heartbeat >= 10 && crying < 50 && crying >= 10){
      stress1 = (crying - 10)/2.25;
      stress2 = (heartbeat - 10);

      //crying has higher priority and reacts quicker, so the distribution has to be changed in its favour
      stressLevel = (0.62 * stress1) +  (0.38 * stress2);
  }
  //for debugging purposes
  else{
      stressLevel = 500;
  }
  return stressLevel;
}


void sendValues(int amplitude, int frequency){
  switchbox_init();

  uart_init(UART0);
  uart_init(UART1);
  // pins for sending
  switchbox_set_pin(IO_AR1 ,SWB_UART0_TX);
  switchbox_set_pin(IO_AR0 ,SWB_UART1_TX);

  pthread_t sendThread;
  uart_index_t port0 = UART0;
  uart_index_t port1 = UART1;

  uint8_t sendF = 0;
  uint8_t sendA = 0;

  // empty placeholder pointers for sending
  uint8_t* sentF = NULL; 
  uint8_t* sentA = NULL;

  sendF = frequency;
  sendA = amplitude;

  // SEND - for test purposes
  // create sender args for frequency transmission
  send_args_t argsF = (send_args_t) {sendF, port0, port0};

  // Send the frequency on a new thread
  pthread_create(&sendThread, NULL, send, &argsF);

  // create sender args for amplitude transmission
  send_args_t argsA = (send_args_t) {sendA, port1, port1};

  // send the amplitude on the main thread
  sentA = (uint8_t*)send(&argsA);
  printf("%d\n", *sentA); // fake use of sentA

  // join the frequency thread before receiving
  pthread_join(sendThread, (void*)&sentF);
  return;
}

void receiveValues(int * heartbeat, int * crying){
  switchbox_init();

  uart_init(UART0);
  uart_init(UART1);
  // pins for receiving
  switchbox_set_pin(IO_AR2 ,SWB_UART0_RX);
  switchbox_set_pin(IO_AR3 ,SWB_UART1_RX);
  pthread_t receiveThread;
  // Variables for the UART peripherals
  uart_index_t port0 = UART0;
  uart_index_t port1 = UART1;
  // the read values for frequency and amplitude to be displayed on screen
  uint8_t readF = 0;
  uint8_t readA = 0;
  //printf("%d %d", readA, readF);
  // placeholder pointer for the amplitude data (from the main thread call)
  uint8_t* readAP = NULL;
  // RECEIVE
  // frequency receive args
  rec_args_t rcArgsF = (rec_args_t) {readF, port0};
  // pointer serving as a placeholder for the received data
  rec_args_t * rcArgsFP = NULL;
  // Get the frequency on a new thread
  pthread_create(&receiveThread, NULL, receive, &rcArgsF);
  // amplitude receive args
  rec_args_t rcArgsA = (rec_args_t) {readA, port1};
  // Get the amplitude on the main thread
  readAP = (uint8_t*)receive(&rcArgsA);
  // join the receive frequency thread before displaying
  pthread_join(receiveThread, (void*)&rcArgsFP);
  // update of variables
  readA = *readAP;
  readF = rcArgsFP->data;
  //printf("%d %d", readA, readF);
  *heartbeat = readA;
  *crying = readF;
  printf("Succesfully received with %d\n", readF);
  return;
}


//function that checks the boundaries of the m.amp and m.freq numbers before sending them to the motor drivers
void numberBoundaries(float * num1, float * num2){
	if(*num1 > 100){
		*num1 = 100;
	}
	if(*num2 > 100){
		*num2 = 100;
	}
	if(*num1 <= 0){
		*num1 = 0;
	}
	if(*num2 <= 0){
		*num2 = 0;
	}
}


int main(){
pynq_init();
int heartRate, cryVolume, currentStress; heartRate = -1; cryVolume = -1, currentStress = 0;

int stressLevel = 0;

display_init(&display);
displayFillScreen(&display, RGB_BLACK);
InitFontx(font, "/home/student/libpynq-5EWC0-2023-v0.2.6/fonts/ILGH16XB.FNT", "");
GetFontx(font, 0, buffer_font, &fontWidth, &fontHeight);
displaySetFontDirection(&display, TEXT_DIRECTION0);

//defining the structure that will hold the values for frequency and amplitude
matrix_t m;
m.freq = MAXIMUM;
m.amp = MAXIMUM;


int randomStress = 0;
int diffStress = 0;

while(heartRate == -1 || cryVolume == -1){
uint8_t textz[] = "Waiting for Input";
displayDrawString(&display, font, 54, 120, textz, RGB_RED);
receiveValues(&heartRate, &cryVolume);
}


while(1){

//send lower frequency
//if no workie, send lower amplitude
//reset when stress difference is too high

//receive values for heartbeat and crying volume
receiveValues(&heartRate, &cryVolume);

//calculate the initial stressLevel before sending a lower frequency
stressLevel = calculateStressLevel(heartRate, cryVolume);

//send a lower frequency and check its boundaries
m.freq = m.freq - 8;
numberBoundaries(&(m.freq), &(m.amp));
sendValues(m.amp, m.freq);

//receive values for heartbeat and crying volume again to compare with the previous
receiveValues(&heartRate, &cryVolume);

//calculate the stress level again after sending lower frequency
currentStress = calculateStressLevel(heartRate, cryVolume);

//calculate the difference between the stress after sending and before sending with respect to the former
diffStress = currentStress - stressLevel;

if(diffStress <= 0){
  //if currentStress is lower than the original stress level nothing is done and the loop is repeated
  //will have to test if sending here is good or not
  // numberBoundaries(&(m.freq), &(m.amp));
  // sendValues(m.amp, m.freq);
}
else if(diffStress > 0 && diffStress <= 3){
  //if the change in stress is minuscule, change nothing and send the same signal
  numberBoundaries(&(m.freq), &(m.amp));
  sendValues(m.amp, m.freq);
}
else if(diffStress > 3 && diffStress <= 8){
  //if currentStress is not much higher than the original stress level, the frequency change is partially undone and a small amplitude change is done
  m.freq = m.freq + 4;
  m.amp = m.amp - 4;
  numberBoundaries(&(m.freq), &(m.amp));
  sendValues(m.amp, m.freq);
}
else if(diffStress > 8){
  //if the currentStress is higher than the original stress level by a margin greater than 5, the frequency change is undone and a lower amplitude is sent
  m.freq = m.freq + 8;
  m.amp = m.amp - 8;
  numberBoundaries(&(m.freq), &(m.amp));
  sendValues(m.amp, m.freq);
}
else if(diffStress > 20){
  //if the currentStress is a lot higher than the original stress level, the amplitude and frequency return to their max values
  m.freq = MAXIMUM;
  m.amp = MAXIMUM;
  numberBoundaries(&(m.freq), &(m.amp));
  sendValues(m.amp, m.freq);
}



printf("Amplitude sent is %.2f. Frequency sent is %.2f.\n", m.amp, m.freq);


displayFillScreen(&display, RGB_BLACK);

    // Display rocking frequency
    printf("Frequency: %.2f\n", m.freq);
    uint8_t text3[] = "Frequency:";
    uint8_t* read_sendF = convertIntegerToASCII(m.freq);
    displayDrawString(&display, font, 100, fontHeight * 1, text3, RGB_WHITE);
    displayDrawString(&display, font, 100, fontHeight * 2, read_sendF, RGB_WHITE);

// Display rocking frequency
    printf("Amplitude: %.2f\n", m.amp);
    uint8_t text4[] = "Amplitude:";
    uint8_t* read_sendA = convertIntegerToASCII(m.amp);
    displayDrawString(&display, font, 100, fontHeight * 4, text4, RGB_WHITE);
    displayDrawString(&display, font, 100, fontHeight * 5, read_sendA, RGB_WHITE);
    
    printf("Stress: %d\n", currentStress);
    uint8_t text5[] = "Stress:";
    uint8_t* read_sendZ = convertIntegerToASCII(randomStress);
    displayDrawString(&display, font, 100, fontHeight * 7, text5, RGB_WHITE);
    displayDrawString(&display, font, 100, fontHeight * 8, read_sendZ, RGB_WHITE);
    
    printf("Stress: %d\n", diffStress);
    uint8_t text6[] = "Stress Diff:";
    uint8_t* read_sendV = convertIntegerToASCII(diffStress);
    displayDrawString(&display, font, 100, fontHeight * 9, text6, RGB_WHITE);
    displayDrawString(&display, font, 100, fontHeight * 10, read_sendV, RGB_WHITE);

    // wait some time for clarity
    sleep_msec(100);






//printf("%d\n", currentStress);
// sleep(1);
}
}
