#include <libpynq.h>
#include <pthread.h>

// how much time to wait between loop cycles (for demonstration)
#define CYCLE 5

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
  sleep_msec(5);
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


int main(void) {
  pynq_init();
  switchbox_init();

  uart_init(UART0);
  uart_init(UART1);
  // pins for sending
  switchbox_set_pin(IO_A5 ,SWB_UART0_TX);
  switchbox_set_pin(IO_A4 ,SWB_UART1_TX);

  // pins for receiving
  switchbox_set_pin(IO_A0 ,SWB_UART0_RX);
  switchbox_set_pin(IO_A1 ,SWB_UART1_RX);

  display_init(&display);
  displayFillScreen(&display, RGB_BLACK);
  InitFontx(font, "/home/student/libpynq-5EWC0-2023-v0.2.2/fonts/ILGH16XB.FNT", "");
  GetFontx(font, 0, buffer_font, &fontWidth, &fontHeight);
  displaySetFontDirection(&display, TEXT_DIRECTION0);

  // two threads for complete safety (not exclusively necessary)
  pthread_t sendThread;
  pthread_t receiveThread;
  
  // Variables for the UART peripherals
  uart_index_t port0 = UART0;
  uart_index_t port1 = UART1;

  // The frequency and amplitude variables being sent (here they are just incremented in the while loop)
  uint8_t sendF = 0;
  uint8_t sendA = 0;

  // empty placeholder pointers for sending
  uint8_t* sentF = NULL; 
  uint8_t* sentA = NULL;

  // the read values for frequency and amplitude to be displayed on screen
  uint8_t readF = 0;
  uint8_t readA = 0;
  
  // placeholder pointer for the amplitude data (from the main thread call)
  uint8_t* readAP = NULL;

  while(1){

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

    sendA = readA;
    sendF = readF;

// SEND - for test purposes
    // create sender args for frequency transmission

    if (sendA > 200 || sendF > 200){
      ;
    }
    else {

  
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
    }
    // Display what was received

    displayFillScreen(&display, RGB_BLACK);

    // Display rocking frequency
    printf("Input1: %d\n", readF);
    uint8_t text1[] = "Input1:";
    uint8_t* read_printF = convertIntegerToASCII(readF);
    displayDrawString(&display, font, 10, fontHeight * 1, text1, RGB_WHITE);
    displayDrawString(&display, font, 10, fontHeight * 2, read_printF, RGB_WHITE);

    // Display rocking amplitude
    printf("Input2: %d\n", readA);
    uint8_t text2[] = "Input2:";
    uint8_t* read_printA = convertIntegerToASCII(readA);
    displayDrawString(&display, font, 10, fontHeight * 4, text2, RGB_WHITE);
    displayDrawString(&display, font, 10, fontHeight * 5, read_printA, RGB_WHITE);

    // Display rocking frequency
    printf("Output1: %d\n", sendF);
    uint8_t text3[] = "Output1:";
    uint8_t* read_sendF = convertIntegerToASCII(sendF);
    displayDrawString(&display, font, 100, fontHeight * 1, text3, RGB_WHITE);
    displayDrawString(&display, font, 100, fontHeight * 2, read_sendF, RGB_WHITE);

// Display rocking frequency
    printf("Output2: %d\n", sendA);
    uint8_t text4[] = "Output2:";
    uint8_t* read_sendA = convertIntegerToASCII(sendA);
    displayDrawString(&display, font, 60, fontHeight * 4, text4, RGB_WHITE);
    displayDrawString(&display, font, 60, fontHeight * 5, read_sendA, RGB_WHITE);

    // wait some time for clarity
    sleep_msec(CYCLE);
  }

  display_destroy(&display);
  uart_destroy(UART0);
  uart_destroy(UART1);
  switchbox_destroy();
  pynq_destroy();
  return EXIT_SUCCESS;
}
