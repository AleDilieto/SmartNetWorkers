int pulsePin = 0;                 // Sensore battito collegato al pin analogico 0

volatile int BPM;                   // Battiti al minuto
volatile int Signal;                // Valore analogico ottenuto dal pin analogico 0
volatile int IBI = 600;             // Intervallo di tempo fra i battiti
volatile boolean Pulse = false;     // Vero quando il battito è in corso
volatile boolean QS = false;        // Vero quando viene trovato un battito

volatile int rate[10];                      // array che memorizza gli ultimi 10 IBI
volatile unsigned long sampleCounter = 0;          // Variabile utilizzata il conteggio del tempo
volatile unsigned long lastBeatTime = 0;           // Variabile utilizzata che contiene il tempo dell'ultimo battito
                                                      // per trovare l'IBI
volatile int P = 512;                      // Variabile utilizzata per trovare il massimo in una pulsazione
volatile int T = 512;                     // Varibile utilizzata per trovare il minimo in una pulsazione
volatile int thresh = 525;                // Variabile utilizzata per trovare il momento esatto del battito
volatile int amp = 100;                   // Ampiezza del battito
volatile boolean firstBeat = true;        // Variabile utilizzata per capire se è il primo battito
volatile boolean secondBeat = false;      // Variabile utilizzata per capire se è il secondo battito

// === SETUP =======================================================================================================
void setup()
{
  Serial.begin(9600);
  while (!Serial) {;}
  interruptSetup();                 // Imposta la lettura del battito cardiaco ogni 2ms
}

// === LOOP ========================================================================================================
void loop()
{
   serialOutput();  
   
  if (QS == true) // Se è stato trovato un battito
    {     
      // (BPM e IBI) sono già stati determinati
      serialOutputWhenBeatHappens(); // Output su seriale    
      QS = false; // Reset di QS per il prossimo battito   
    }
   
  delay(20);
}

// === INTERRUPT SETUP =============================================================================================
void interruptSetup()
{     
  // Inizializza il timer per lanciare un interrupt ogni 2ms
  TCCR2A = 0x02;
  TCCR2B = 0x06;
  OCR2A = 0X7C;
  TIMSK2 = 0x02;
  sei();             // Abilitazione interrupt     
} 

// === SERIAL OUTPUT ===============================================================================================
void serialOutput()
{
  //sendDataToSerial('S', Signal);     // Invia i dati alla seriale    
}

// === SERIAL OUTPUT WHEN BEAT HAPPENS =============================================================================
void serialOutputWhenBeatHappens()
{    
 
     sendDataToSerial('B',BPM);
     //sendDataToSerial('Q',IBI);
}

// === SEND DATA TO SERIAL =========================================================================================
void sendDataToSerial(char symbol, int data )
{
   Serial.print(symbol);
   Serial.println(data);                
}

// === ISR =========================================================================================================
ISR(TIMER2_COMPA_vect) // Interrupt Service Routine
{  
  cli();                                      // Disabilitazione interrupt
  Signal = analogRead(pulsePin);              // Lettura segnale analogico
  sampleCounter += 2;                         // Aggiornamento tempo
  int N = sampleCounter - lastBeatTime;       // Tempo dall'ultimo battito

  // Ricerca del minimo della pulsazione
  // (thresh rappresenta il valor medio del battito)
  if(Signal < thresh && N > (IBI/5)*3) // Si aspetta 3/5 dell'ultimo IBI per evitare disturbi
    {      
      if (Signal < T)
      {                        
        T = Signal;
      }
    }

  // Ricerca del massimo della pulsazione
  if(Signal > thresh && Signal > P)
    {
      P = Signal;
    }

  // Ricerca del battito cardiaco
  if (N > 250)
  {
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ) // Per evitare disturbi
      {       
        //Serial.println("Trovato");
        Pulse = true;                               // Si pensa di aver trovato un battito
        IBI = sampleCounter - lastBeatTime;         // Calcolo dell'IBI
        lastBeatTime = sampleCounter;               // Aggiornamento del tempo dell'ultimo battito
  
        if(secondBeat) // Se è il secondo battito
        {
          secondBeat = false;                  
          for(int i=0; i<=9; i++) // Inizializza il vettore di IBI
          {             
            rate[i] = IBI;                      
          }
        }
  
        if(firstBeat) // Se è la prima volta che si trova una pulsazione
        {                         
          firstBeat = false;                   
          secondBeat = true;                   
          sei();                               // Abilitazione interrupt
          return;                              // Essendo la prima pulsazione,
                                                  // IBI non è affidabile e quindi viene scartato
        }   
      
      word runningTotal = 0;                  // Totale degli IBI, inizializzato a 0   

      for(int i=0; i<=8; i++) // Gli ultimi 10 IBI vengono shiftati di 1
        {
          rate[i] = rate[i+1];
          runningTotal += rate[i];              // Viene calcolato il totale degli IBI
        }

      rate[9] = IBI;                          // Aggiunta dell'ultimo IBI all'array di IBI
      runningTotal += rate[9];                // Aggiunta dell'ultimo IBI al totale degli IBI
      runningTotal /= 10;                     // Calcolo della media degli ultimi 10 IBI
      BPM = 60000/runningTotal;               // Calcolo BPM (60000 ms = 1 min)
      QS = true;                              // E' stato trovato un battito
    }                       
  }

  if (Signal < thresh && Pulse == true) // Quando i valori diventano bassi, la pulsazione è finita
    {
      Pulse = false;                         // Reset variabile Pulse
      amp = P - T;                           // Calcolo ampiezza del battito
      thresh = amp/2 + T;                    // Impostazione thresh alla metà dell'ampiezza
      P = thresh;                            // Reset massimo
      T = thresh;                            // Reset minimo
    }

  if (N > 2500) // Se sono passati più di 2 secondi dall'ultimo battito
    {
      thresh = 512;                          // Reset thresh
      P = 512;                               // Reset massimo
      T = 512;                               // Reset minimo
      lastBeatTime = sampleCounter;          // La variabile lastBeatTime deve essere mantenuta "al passo"
      
      // Reset di firstBeat e secondBeat per evitare disturbi al prossimo battito 
      firstBeat = true;
      secondBeat = false;
    }

  sei();                                   // Abilitazione interrupt
}
