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


typedef struct _diff1{
    int difference;
    int a;
    int b;
} diff1;

typedef struct _diff2{
    int difference;
    int a;
    int b;
} diff2;


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

void red () {
  printf("\033[1;31m");
}
void purple () {
  printf("\033[1;35m");
}
void cyan () {
  printf("\033[1;36m");
}
void yellow () {
  printf("\033[1;33m");
}
void green () {
  printf("\033[1;32m");
}

void reset () {
  printf("\033[0m");
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
        

void sendValues(int amplitude, int frequency){

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
  printf("Succesfully received with %d\n", readA);
  printf("Succesfully received with %d\n", readF);
  return;
}



void randomNumber(int * frequency, int * amplitude){
  int ran1, ran2; ran1 = 0; ran2 = 0;

  ran1 = rand() % 101;
  ran2 = rand() % 101;

  *frequency = ran1;
  *amplitude = ran2;

}

void displayFunction(int a, int b){
    displayFillScreen(&display, RGB_BLACK);

    // displayDrawRect(&display, 20, 70, 70, 20, RGB_WHITE);
    // displayDrawRect(&display, 20, 150, 70, 100, RGB_WHITE);
    // displayDrawRect(&display, 20, 230, 70, 180, RGB_WHITE);

    // displayDrawRect(&display, 90, 70, 140, 20, RGB_WHITE);
    // displayDrawRect(&display, 90, 150, 140, 100, RGB_WHITE);
    // displayDrawRect(&display, 90, 230, 140, 180, RGB_WHITE);

    // displayDrawRect(&display, 160, 70, 210, 20, RGB_WHITE);
    // displayDrawRect(&display, 160, 150, 210, 100, RGB_WHITE);
    // displayDrawRect(&display, 160, 230, 210, 180, RGB_WHITE);

    uint16_t startX = 25;
    uint16_t startY = 20;
    uint16_t rectWidth = 30;
    uint16_t rectHeight = 30;
    uint16_t rectSpacing = 10;

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            uint16_t x1 = startX + col * (rectWidth + rectSpacing);
            uint16_t y1 = startY + row * (rectHeight + rectSpacing);
            uint16_t x2 = x1 + rectWidth;
            uint16_t y2 = y1 + rectHeight;

            displayDrawRect(&display, x1, y1, x2, y2, RGB_WHITE);
        }
    }







    printf(" %d %d", a ,b);

    // uint8_t text3[] = "A:";
    // uint8_t* aInt = convertIntegerToASCII(a);
    // displayDrawString(&display, font, 20, fontHeight * 1, text3, RGB_WHITE);
    // displayDrawString(&display, font, 20, fontHeight * 2, aInt, RGB_WHITE);

    // // Display rocking frequency
    // //printf("Amplitude: %.2f\n", m->amp);
    // uint8_t text4[] = "B:";
    // uint8_t* bInt = convertIntegerToASCII(b);
    // displayDrawString(&display, font, 20, fontHeight * 4, text4, RGB_WHITE);
    // displayDrawString(&display, font, 20, fontHeight * 5, bInt, RGB_WHITE);

}

void pathMaker1(int a, int b, int* heartbeat, int* crying){
    switchbox_init();

    uart_init(UART0);
    uart_init(UART1);
    // pins for receiving
    switchbox_set_pin(IO_AR2 ,SWB_UART0_RX);
    switchbox_set_pin(IO_AR3 ,SWB_UART1_RX);
    switchbox_set_pin(IO_AR1 ,SWB_UART0_TX);
    switchbox_set_pin(IO_AR0 ,SWB_UART1_TX);

    uint8_t heartbeat12 = 0;

    uint8_t heartB = 0;

    int lastdir = 1;


    displayFunction(a, b);
    printf("function started");
    //int prevH;prevH = 0; 

    int diffHeartbeat1, diffHeartbeat2; diffHeartbeat1 = 0; diffHeartbeat2 = 0; 

    int freq[5] = {10, 33, 55, 78, 100};
    int amp[5] = {10, 33, 55, 78, 100};
    
    red();
    printf("Amplitude %d, Frequency %d\n", amp[a-1], freq[b-1]);
    reset();
    sendValues(amp[a-1], freq[b-1]);
    printf("a is %d, b is %d\n", a, b);

    sleep_msec(20000);

    uart_reset_fifos(UART1);
    heartB = uart_recv(UART1);

    while(1){
    if(a < 1){
        a = 1;
    }
    else if(a > 5){
        a = 5;
    }
    if(b < 1){
        b = 1;
    }
    else if(b > 5){
        b = 5;
    }


    if(a == 1 && b == 1){
        green();
        printf("con");
        reset();
        yellow();
        printf("gr");
        reset();
        purple();
        printf("ats!\n\n\n\n\n");
        reset();
        return;
    }

    printf("Heartbeat is %d, Crying Volume is %d\n", *heartbeat, *crying);

  
    if(a == 1 && b >= 4){
        a = 5;
        b = 5;

    }



    switch(lastdir){
      case 0:
        if(b>1){
        b = b - 1;
        sendValues(amp[a-1], freq[b-1]);
        lastdir = 1;
        break;
        }
        else{
          a = a - 1;
          sendValues(amp[a-1], freq[b-1]);
        }
        break;
      case 1:
        if(a > 1){
        a = a -1;
        sendValues(amp[a-1], freq[b-1]);
        lastdir = 0;
        }
        else{
          b = b - 1;
          sendValues(amp[a-1], freq[b-1]);
        }
        break;
    }

    //sleep_msec(5000);


    //randomNumber(heartbeat, crying);
    diffHeartbeat1 = heartB;
    printf("before\n");
    sleep(13);
    printf("after\n");

    uart_reset_fifos(UART1);
    heartbeat12 = uart_recv(UART1);

    printf("Heartbeat12 is %d", heartbeat12);
    
    //randomNumber(heartbeat, crying);
    diffHeartbeat2 = heartbeat12;
    



    int averageHeartbeat = diffHeartbeat2 - diffHeartbeat1;

      cyan();
      printf("averageHeartbeat is %d\n", averageHeartbeat);
      reset();
    
    if(averageHeartbeat >= 0){
      switch(averageHeartbeat){
      case 1:
        b = b + 1;
        sendValues(amp[a-1], freq[b-1]);
        break;
      case 0:
        a = a + 1;
        sendValues(amp[a-1], freq[b-1]);
        break;
      }
    }



    //displayFunction(a, b);
    //remove sleep in later edition of the code
    //sleep(2);
    heartB = heartbeat12;
    sleep_msec(5000);
  }

    return;  

}





int main(void){
    pynq_init();

    int a, b; a = 5; b = 5;
    //int c = 0;

    int heartbeat, crying; heartbeat = 100; crying = 100;

    display_init(&display);
    displayFillScreen(&display, RGB_BLACK);
    InitFontx(font, "/home/student/libpynq-5EWC0-2023-v0.2.6/fonts/ILGH16XB.FNT", "");
    GetFontx(font, 0, buffer_font, &fontWidth, &fontHeight);
    displaySetFontDirection(&display, TEXT_DIRECTION0);


    printf("Starting loop!\n");

    pathMaker1(a, b, &heartbeat, &crying);

    //printf("%d", c);

    //uint8_t curr = 0;
    //uint8_t last = 0;







    return 0;
}
