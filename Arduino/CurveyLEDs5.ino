// Original base: http://www.instructables.com/id/Arduino-Timer-Interrupts/

// timer setup for timer0, timer1, and timer2.
//For arduino uno or any board with ATMEL 328/168.. diecimila, duemilanove, lilypad, nano, mini...

//#define DONT_USE_INTERRUPTS

// Set DIRECTION true for right and false for left
#define DIRECTION false

#ifndef DONT_USE_INTERRUPTS
//this code will enable all three arduino timer interrupts.
//timer0 will interrupt at 2kHz
//timer1 will interrupt at 1Hz
//timer2 will interrupt at 8kHz

//#define pinTimer0  8
#define pinTimer1  13
//#define pinTimer2  9

#endif // not DONT_USE_INTERRUPTS

#define LED_MODULE_COUNT 4                      // Number of LED units chained together into one display
#define MAX_ROWS         7                      // Number of pixel rows per LED unit
#define MAX_COLUMNS      32 * LED_MODULE_COUNT  // Max number of pixels columns in total display
#define MAX_BYTES        MAX_COLUMNS / 8        // Max width of the display buffer in bytes

// Curvey LED pins
// Serial-data input to the Shift Register via sdataPin
int sdataPin =5;
// Clock input for data shift on rising edge
int clockPin =4;
// Serial data is transferred to the respective latch
// when latchPin (LE) is high. The data is latched when
// latchPin goes low. Also, a control signal input for
// Current Adjust mode.
int latchPin =3;
// When (active) low, the output drivers are enabled;
// when high, all output drivers are turned OFF (blanked).
// Also, a second control signal input for Current
// Adjust mode
int enablePin=2;
int rowPin[MAX_ROWS] = {6, 7, 8, 9, 10, 11, 12};

// Each byte represents the 7 rows of a column (plus a spare bit)
// Columns are outbut in reverse order, so index 0 is output first
// and eventually gets scrolled to the last column in the LED display.
byte colBuffer[MAX_COLUMNS];

#define FONT_WIDTH 5                            // Pixel width of the font
namespace
{
  // The printable ASCII characters only (32-126)
  extern byte font[95][FONT_WIDTH];    // Defined at end of the source
}

//storage variables
boolean toggle0 = 0;
boolean toggle1 = 0;
boolean toggle2 = 0;


// Display the buffer to the serial line in reverse order
// compare to outputting to the LED display.
void DisplayLedBuffer()
{
  byte rowMask = 0x01;
  for (int i = 0; i < MAX_ROWS; ++i)
  {
    for (int j = MAX_COLUMNS - 1; j >= 0; --j)
    {
      if (colBuffer[j] & rowMask)
        Serial.print('*');
      else
        Serial.print(' ');
    }
    Serial.println(" ");
    rowMask <<= 1;    // Set bit representing next row in the column
  }
  Serial.println("-----------------------------");
  Serial.println(" ");
  Serial.println(" ");
}

void ClearLedBufferRight(int fromCol)
{
  int toCol = MAX_COLUMNS - fromCol;
  for (int i = 0; i < MAX_COLUMNS; ++i)
  {
    if (i < toCol)
      colBuffer[i] = 0;
  }
}

byte nLastRowWritten = MAX_ROWS - 1;
byte nTurnOffRow;

void WriteLedRowToPins()
{
  // Turn off output before we latch in the next set of values
  nTurnOffRow = nLastRowWritten++;
//  digitalWrite(enablePin, HIGH);
  // disable the previously enabled row
//  digitalWrite(rowPin[nLastRowWritten++], LOW);
//  digitalWrite(enablePin, LOW);
  if (nLastRowWritten >= MAX_ROWS)
    nLastRowWritten = 0;

  // For next row in the LED pixel buffer ...
  byte rowMask = 0x01 << nLastRowWritten;
  
  // Note that latchPin should be LOW here
  for (int i = 0; i < MAX_COLUMNS; ++i)
  {
    // for each column
    // Clock values in for each LED of the row
    digitalWrite(clockPin, LOW);
    digitalWrite(sdataPin, ((colBuffer[i] & rowMask) ? HIGH : LOW));
    digitalWrite(clockPin, HIGH);
  }
  digitalWrite(clockPin, LOW);  // needed?
  // Turn off output before we latch in the next set of values
  digitalWrite(enablePin, HIGH);
    for (int k = 0 ; k < MAX_ROWS; ++k)
      digitalWrite(rowPin[k], LOW);
  // Latch the values
  digitalWrite(latchPin, HIGH);
  digitalWrite(latchPin, LOW);

  // Turn off previous row
//  digitalWrite(rowPin[nTurnOffRow], LOW);
  // Enable the row we want
  digitalWrite(rowPin[nLastRowWritten], HIGH);
  digitalWrite(enablePin, LOW);
}

void setBufferFromString(char *pString)
{
  int nStringPosn = 0;
  
  for (int nColumnIndex = 0; nColumnIndex < MAX_COLUMNS;)
  {
    // Copy string until terminator, then pad with spaces
    if (pString[nStringPosn] != 0)
      nColumnIndex = setPixelsFromChar(pString[nStringPosn++], nColumnIndex);
    else
      nColumnIndex = setPixelsFromChar(' ', nColumnIndex);
  }
//  Serial.println(" ");
//  Serial.println("----------------------------");
}

// Set the character into the buffer, starting at the given pixel colummn and
// returning the next column to use, after adding a blank separating column.
int setPixelsFromChar(char c, int nStartColumn)
{
  int index = c - 32;  // Set the font table index for the current char
  
  if (index < 0 || index > 126)
    index = 0;

  byte (*pFontCharColumns)[FONT_WIDTH] = &font[index];

  int nCurrColumn = MAX_COLUMNS - nStartColumn - 1;
  // If first col of char is space then skip it;
  int i = ((*pFontCharColumns)[0] != 0) ? 0 : 1;
  for (; i < FONT_WIDTH && nCurrColumn >= 0; ++i)
  {
    // for each col of the font
    colBuffer[nCurrColumn--] = (*pFontCharColumns)[i];
  }

  // If last col of char is not space then add a blank column
  if (nCurrColumn >= 0 && (*pFontCharColumns)[FONT_WIDTH] != 0)
//  if (nCurrColumn >= 0 && c != ' ')
  {
    // Add a blank, spacer column
    colBuffer[nCurrColumn--] = 0;
  }
  return MAX_COLUMNS - nCurrColumn - 1;
}

char * m_pString      = NULL;
byte (*m_pFontChar)[FONT_WIDTH] = NULL;
int    m_nStringIndex  = 0;
int    m_nStringLength = 0;    // ie. max index + 1
byte   m_nFontCol      = 0;

void SetStringToSlide(char *pString, bool bIsRight = true)
{
  m_pString = pString;

  // Get the string length (ie. index + 1)
  for (m_nStringLength = 0; m_pString[m_nStringLength] != 0; ++m_nStringLength);
  
  if (bIsRight)
    m_nStringIndex = m_nStringLength - 1;
  else
    m_nStringIndex = 0;
 
  int index = m_pString[m_nStringIndex] - 32;  // Set the font table index for the current char
  
  if (index < 0 || index > 126)
    index = 0;
  m_pFontChar = &font[index];
  if (bIsRight)
    m_nFontCol = FONT_WIDTH;  // ie. one beyond last font column (origin 0)
  else
    m_nFontCol = 0;           // First col of first char
    
  Serial.print("Index: ");
  Serial.print(m_nStringIndex);
  Serial.print(" Column: ");
  Serial.println(m_nFontCol);
}

// Returns true if there is another column left to display
bool IncrementSlideString(bool bIsRight = true)
{
  if (bIsRight)
    IncrementSlideStringRight();
  else
    IncrementSlideStringLeft();
}

// Returns true if there is another column left to display
bool IncrementSlideStringRight()
{
  switch (m_nFontCol)
  {
  case -1:
    break;  // Nothing to do
  case 0:
    // Check if we have completed the string
    if (m_nStringIndex == 0)
    {
      // Check if we suppress a blank column
      if ((*m_pFontChar)[0] != 0)
        InsertColumnRight((*m_pFontChar)[0]);
      m_nFontCol = -1;
      return false;    // End of the string
    }
    // Print the last column for the char.
    InsertColumnRight((*m_pFontChar)[0]);
    // If it is non-blank then we will add an extra blank.
    // Check if we suppress a blank column that otherwise follows a non-blank col 0
    if ((*m_pFontChar)[0] == 0)
      m_nFontCol = FONT_WIDTH - 1;  // Last column of font
    else
      m_nFontCol = FONT_WIDTH;      // ie. one beyond last font column (origin 0)
    {
      // Get previous char in string
      int index = m_pString[--m_nStringIndex] - 32;  // Set the font table index for the current char
      
      if (index < 0 || index > 126)
        index = 0;
      m_pFontChar = &font[index];
    }
    break;
  case FONT_WIDTH - 1:  // Last column of font
    // Check if we need a blank column
    if ((*m_pFontChar)[FONT_WIDTH - 1] == 0)
    {
      // Suppress blank col, use the next
      --m_nFontCol;
    }
    InsertColumnRight((*m_pFontChar)[m_nFontCol--]);
    break;
  case FONT_WIDTH:  // ie. one beyond last font column (origin 0)
    // Always Insert a blank column
    InsertColumnRight(0);
    --m_nFontCol;
    break;
  default:
    InsertColumnRight((*m_pFontChar)[m_nFontCol--]);
    break;
  }
  return true;
}

void InsertColumnRight(byte colData)
{
  // Shift right 1 column
  for (int i = 1; i < MAX_COLUMNS; ++i)
    colBuffer[i - 1] = colBuffer[i];

  colBuffer[MAX_COLUMNS - 1] = colData;
}

void InsertColumnLeft(byte colData)
{
  // Shift left 1 column
  for (int i = MAX_COLUMNS - 2; i >= 0; --i)
    colBuffer[i + 1] = colBuffer[i];

  colBuffer[0] = colData;
}

// Returns true if there is another column left to display
bool IncrementSlideStringLeft()
{
  switch (m_nFontCol)
  {
  case -1:
    break;  // Nothing to do
  case FONT_WIDTH - 1:  // Last column of font
    // Check if we have completed the string
    if (++m_nStringIndex >= m_nStringLength)
    {
      InsertColumnLeft((*m_pFontChar)[FONT_WIDTH - 1]);
      m_nFontCol = -1;
      return false;    // End of the string
    }
    // Check if we need a blank column
    if ((*m_pFontChar)[FONT_WIDTH - 1] != 0)
    {
      // Yes, we need an extra blank column
      InsertColumnLeft((*m_pFontChar)[FONT_WIDTH - 1]);
      ++m_nFontCol;
      break;
    }
    // Suppress last column & drop thru
  case FONT_WIDTH:  // ie. one beyond last font column (origin 0)
    // Always Insert a blank column
    InsertColumnLeft(0);
    // Set next char in string, m_nStringIndex
    // has previously been incremented
    {
      int index = m_pString[m_nStringIndex] - 32;  // Set the font table index for the current char
      
      if (index < 0 || index > 126)
        index = 0;
      m_pFontChar = &font[index];
    }
    // Check if we suppress a blank first column
    if ((*m_pFontChar)[0] == 0)
      m_nFontCol = 1;
    else
      m_nFontCol = 0;
      
    break;
  case 0:
    // If we come here it means we have already decided
    // not to suppress the first column if blank.
  default:
    InsertColumnLeft((*m_pFontChar)[m_nFontCol++]);
    break;
  }
  
  return true;
}

//***************************************************************************
void setup()
{
#ifdef DONT_USE_INTERRUPTS  
  Serial.begin(9600);
  Serial.flush();
  Serial.println(' ');

  delay(1000);
  char * str1 = "A (!) string ";
  
//  setString(str1, 0);
//  DisplayLedBuffer();
  
  setBufferFromString(str1);
  DisplayLedBuffer();
  
  SetStringToSlide("Kaplunk? ", DIRECTION);
#else
    //set pins to output 
  pinMode(sdataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  for (int i = 0 ; i < MAX_ROWS; ++i)
    pinMode(rowPin[i], OUTPUT);

//  SetStringToSlide("This long string is really Rubbish! ", DIRECTION);
  setBufferFromString("    Swindon Hackspace ");

//  char * str1 = "A (!) string ";
//  setBufferFromString(str1);

  //set pins as outputs
//  pinMode(pinTimer0, OUTPUT);
  pinMode(pinTimer1, OUTPUT);
//  pinMode(pinTimer2, OUTPUT);

  cli();//stop interrupts
  
  //set timer0 interrupt at 2kHz
  TCCR0A = 0;// set entire TCCR2A register to 0
  TCCR0B = 0;// same for TCCR2B
  TCNT0  = 0;//initialize counter value to 0
  // We can use the prescaler to give more control over the interrupt frequncy:
  // interrupt frequency (Hz) = (Arduino clock speed 16,000,000Hz) / (prescaler * (compare match register + 1))
  // or (from http://www.instructables.com/id/Arduino-Timer-Interrupts/)
  // compare match register = [ 16,000,000Hz/ (prescaler * desired interrupt frequency) ] - 1
  // remember that when you use timers 0 and 2 this number must be less than 256, and less than 65536 for timer1
  //
  //    Clock select bit description
  //    CS02 CS01 CS00 	Divisor 
  //      0    0    0     No clock source (timer stopped)
  //      0    0    1 	1 
  //      0    1    0 	8 
  //      0    1    1	64 
  ///     1    0    0	256
  //      1    0    1 	1024
  //
#ifdef INTERRUPT_0_2KHZ
  // set compare match register for 2khz increments
  OCR0A = 124;// = (16*10^6) / (2000*64) - 1 (must be <256)
  // turn on CTC mode
  TCCR0A |= (1 << WGM01);
  // Set CS01 and CS00 bits for 64 prescaler
  TCCR0B |= (1 << CS01) | (1 << CS00);   
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);
#else
  // set compare match register for 128hz increments
  OCR0A = 122;// = (16*10^6) / (128*1024) - 1 (must be <256)
  OCR0A = 31;// = (16*10^6) / (128*1024) - 1 (must be <256)
  // turn on CTC mode
  TCCR0A |= (1 << WGM01);
  // Set CS02 and CS00 bits for 1024 prescaler
  TCCR0B |= (1 << CS02) | (1 << CS00);   
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);
#endif

  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536) : 1Hz
  OCR1A = 7812;// = (16*10^6) / (1*1024) - 1 (must be <65536)  : 2Hz
//OCR1A = 3906;// = (16*10^6) / (1*1024) - 1 (must be <65536)  : 4Hz
//OCR1A = 1953;// = (16*10^6) / (1*1024) - 1 (must be <65536)  : 8Hz
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
/*
//set timer2 interrupt at 8kHz
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  OCR2A = 249;// = (16*10^6) / (8000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 8 prescaler
  TCCR2B |= (1 << CS21);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
  */

  sei();//allow interrupts
  
#endif     // DONT_USE_INTERRUPTS
}  //end setup


#ifdef DONT_USE_INTERRUPTS  

//***************************************************************************

void loop()
{
  char * str1 = "Kaplunk?";
  /*
  for (int i = 0; i < 50; ++i)
  {
    if (i == 0)
    {
      delay(10000);
//      ClearLedBufferRight(0);
//      setString(str1, 0);
      setBufferFromString(str1);
      DisplayLedBuffer();
    }
  }
  */
  while (IncrementSlideString(DIRECTION))
  {
    DisplayLedBuffer();

//  delay(4 * 1000);
  }
   DisplayLedBuffer();
  delay(10 * 1000);
  SetStringToSlide("This long string is really Rubbish! ", DIRECTION);
}

#else

void loop()
{
//do other things here
//displayBounce();
}


ISR(TIMER0_COMPA_vect)
{//timer0 interrupt 2kHz toggles pin 8
//generates pulse wave of frequency 2kHz/2 = 1kHz (takes two cycles for full wave- toggle high then toggle low)
/*
  if (toggle0)
  {
    digitalWrite(pinTimer0,HIGH);
    toggle0 = 0;
  }
  else
  {
    digitalWrite(pinTimer0,LOW);
    toggle0 = 1;
  }
*/
 WriteLedRowToPins();
}

ISR(TIMER1_COMPA_vect)
{//timer1 interrupt 1Hz toggles pin 13 (LED)
//generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)

  if (toggle1)
  {
    digitalWrite(pinTimer1,HIGH);
    toggle1 = 0;
  }
  else
  {
    digitalWrite(pinTimer1,LOW);
    toggle1 = 1;
  }
  
//  if (!IncrementSlideString(DIRECTION))
//    SetStringToSlide("End of Cheese Error!!!", DIRECTION);
//WriteLedRowToPins();


}
  
ISR(TIMER2_COMPA_vect)
{//timer1 interrupt 8kHz toggles pin 9
//generates pulse wave of frequency 8kHz/2 = 4kHz (takes two cycles for full wave- toggle high then toggle low)
/*
  if (toggle2)
  {
    digitalWrite(pinTimer2,HIGH);
    toggle2 = 0;
  }
  else
  {
    digitalWrite(pinTimer2,LOW);
    toggle2 = 1;
  }
  */
}

#endif // DONT_USE_INTERRUPTS

namespace
{
  // The printable ASCII characters only (32-126)
  byte font[][5] = {      //Numeric Font Matrix (Arranged as 7x font data + 1x kerning data)
      {0x00, 0x00, 0x00, 0x00, 0x00},   // sp
      {0x00, 0x00, 0x2f, 0x00, 0x00},   // !
      {0x00, 0x07, 0x00, 0x07, 0x00},   // "
      {0x14, 0x7f, 0x14, 0x7f, 0x14},   // #
      {0x24, 0x2a, 0x7f, 0x2a, 0x12},   // $
      {0x61, 0x66, 0x08, 0x33, 0x43},   // %
      {0x36, 0x49, 0x55, 0x22, 0x50},   // &
      {0x00, 0x05, 0x03, 0x00, 0x00},   // '
      {0x00, 0x1c, 0x22, 0x41, 0x00},   // (
      {0x00, 0x41, 0x22, 0x1c, 0x00},   // )
      {0x14, 0x08, 0x3E, 0x08, 0x14},   // *
      {0x08, 0x08, 0x3E, 0x08, 0x08},   // +
      {0x00, 0x00, 0x50, 0x30, 0x00},   // ,
      {0x10, 0x10, 0x10, 0x10, 0x10},   // -
      {0x00, 0x60, 0x60, 0x00, 0x00},   // .
      {0x20, 0x10, 0x08, 0x04, 0x02},   // /
      {0x3E, 0x51, 0x49, 0x45, 0x3E},   // 0
      {0x00, 0x42, 0x7F, 0x40, 0x00},   // 1
      {0x42, 0x61, 0x51, 0x49, 0x46},   // 2
      {0x21, 0x41, 0x45, 0x4B, 0x31},   // 3
      {0x18, 0x14, 0x12, 0x7F, 0x10},   // 4
      {0x27, 0x45, 0x45, 0x45, 0x39},   // 5
      {0x3C, 0x4A, 0x49, 0x49, 0x30},   // 6
      {0x01, 0x71, 0x09, 0x05, 0x03},   // 7
      {0x36, 0x49, 0x49, 0x49, 0x36},   // 8
      {0x06, 0x49, 0x49, 0x29, 0x1E},   // 9
      {0x00, 0x36, 0x36, 0x00, 0x00},   // :
      {0x00, 0x56, 0x36, 0x00, 0x00},   // ;
      {0x08, 0x14, 0x22, 0x41, 0x00},   // <
      {0x14, 0x14, 0x14, 0x14, 0x14},   // =
      {0x00, 0x41, 0x22, 0x14, 0x08},   // >
      {0x02, 0x01, 0x51, 0x09, 0x06},   // ?
      {0x32, 0x49, 0x59, 0x51, 0x3E},   // @
      {0x7E, 0x11, 0x11, 0x11, 0x7E},   // A
      {0x7F, 0x49, 0x49, 0x49, 0x36},   // B *
      {0x3E, 0x41, 0x41, 0x41, 0x22},   // C *
      {0x7F, 0x41, 0x41, 0x22, 0x1C},   // D *
      {0x7F, 0x49, 0x49, 0x49, 0x41},   // E *
      {0x7F, 0x09, 0x09, 0x09, 0x01},   // F
      {0x3E, 0x41, 0x49, 0x49, 0x7A},   // G
      {0x7F, 0x08, 0x08, 0x08, 0x7F},   // H
      {0x00, 0x41, 0x7F, 0x41, 0x00},   // I
      {0x20, 0x40, 0x41, 0x3F, 0x01},   // J
      {0x7F, 0x08, 0x14, 0x22, 0x41},   // K
      {0x7F, 0x40, 0x40, 0x40, 0x40},   // L
      {0x7F, 0x02, 0x0C, 0x02, 0x7F},   // M
      {0x7F, 0x04, 0x08, 0x10, 0x7F},   // N
      {0x3E, 0x41, 0x41, 0x41, 0x3E},   // O
      {0x7F, 0x09, 0x09, 0x09, 0x06},   // P
      {0x3E, 0x41, 0x51, 0x21, 0x5E},   // Q
      {0x7F, 0x09, 0x19, 0x29, 0x46},   // R
      {0x46, 0x49, 0x49, 0x49, 0x31},   // S
      {0x01, 0x01, 0x7F, 0x01, 0x01},   // T
      {0x3F, 0x40, 0x40, 0x40, 0x3F},   // U
      {0x1F, 0x20, 0x40, 0x20, 0x1F},   // V
      {0x3F, 0x40, 0x38, 0x40, 0x3F},   // W
      {0x63, 0x14, 0x08, 0x14, 0x63},   // X
      {0x07, 0x08, 0x70, 0x08, 0x07},   // Y
      {0x61, 0x51, 0x49, 0x45, 0x43},   // Z
      {0x00, 0x7F, 0x41, 0x41, 0x00},   // [
      {0x55, 0x2A, 0x55, 0x2A, 0x55},   // checker pattern
      {0x00, 0x41, 0x41, 0x7F, 0x00},   // ]
      {0x04, 0x02, 0x01, 0x02, 0x04},   // ^
      {0x40, 0x40, 0x40, 0x40, 0x40},   // _
      {0x00, 0x01, 0x02, 0x04, 0x00},   // '
      {0x20, 0x54, 0x54, 0x54, 0x78},   // a
      {0x7F, 0x48, 0x44, 0x44, 0x38},   // b
      {0x38, 0x44, 0x44, 0x44, 0x28},   // c
      {0x38, 0x44, 0x44, 0x48, 0x7F},   // d
      {0x38, 0x54, 0x54, 0x54, 0x18},   // e
      {0x08, 0x7E, 0x09, 0x01, 0x02},   // f
      {0x0C, 0x52, 0x52, 0x52, 0x3E},   // g
      {0x7F, 0x08, 0x04, 0x04, 0x78},   // h
      {0x00, 0x44, 0x7D, 0x40, 0x00},   // i
      {0x20, 0x40, 0x44, 0x3D, 0x00},   // j
      {0x7F, 0x10, 0x28, 0x44, 0x00},   // k
      {0x00, 0x41, 0x7F, 0x40, 0x00},   // l
      {0x78, 0x04, 0x18, 0x04, 0x78},   // m
      {0x7C, 0x08, 0x04, 0x04, 0x78},   // n
      {0x38, 0x44, 0x44, 0x44, 0x38},   // o
      {0x7C, 0x14, 0x14, 0x14, 0x08},   // p
      {0x08, 0x14, 0x14, 0x18, 0x7C},   // q
      {0x7C, 0x08, 0x04, 0x04, 0x08},   // r
      {0x48, 0x54, 0x54, 0x54, 0x20},   // s
      {0x04, 0x3F, 0x44, 0x40, 0x20},   // t
      {0x3C, 0x40, 0x40, 0x20, 0x7C},   // u
      {0x1C, 0x20, 0x40, 0x20, 0x1C},   // v
      {0x3C, 0x40, 0x30, 0x40, 0x3C},   // w
      {0x44, 0x28, 0x10, 0x28, 0x44},   // x
      {0x0C, 0x50, 0x50, 0x50, 0x3C},   // y
      {0x44, 0x64, 0x54, 0x4C, 0x44},   // z
      {0x00, 0x08, 0x36, 0x41, 0x41},   // {
      {0x00, 0x00, 0x7F, 0x00, 0x00},   // |
      {0x00, 0x41, 0x41, 0x36, 0x08},   // }
      {0x00, 0x06, 0x09, 0x09, 0x06}    // Degree symbol
  };
}


void displayBounce()
{
//  for (int i = 0; i < 10; ++i)
    displayBall();
  moveBall();
    displayBall();
  moveBall();
    displayBall();
  moveBall();
    displayBall();
  moveBall();
    displayBall();
  moveBall();
    displayBall();
  moveBall();
//  delay(100000);
}

#define BALL_PARK_WIDTH 32

byte posn_x = 0;
bool isUp = false;
byte posn_y = 0;
bool isLeft = false;

void displayBall()
{
  for (int i = 0; i < MAX_ROWS; ++i)
  {
    // for each row
    for (int j = 0; j < BALL_PARK_WIDTH; j++)
    {
      // for each column
      // Clock values in for each LED of the row
      digitalWrite(clockPin, LOW);
      digitalWrite(sdataPin, (i == posn_x && j == posn_y) ? HIGH : LOW);
      digitalWrite(clockPin, HIGH);

/*
      if (i == posn_x && j == posn_y)
        Serial.print('*');
      else
        Serial.print(' ');
*/
    }
//    Serial.println(' ');
    digitalWrite(clockPin, LOW);  // needed?
    // Turn off output before we latch in the next set of values
    digitalWrite(enablePin, HIGH);
    // Disable all rows
    for (int k = 0 ; k < MAX_ROWS; ++k)
      digitalWrite(rowPin[k], LOW);
    digitalWrite(rowPin[i], LOW);
    // Latch the values
    digitalWrite(latchPin, HIGH);
    digitalWrite(latchPin, LOW);
  
    // Enable the row we want
    digitalWrite(rowPin[i], HIGH);
    digitalWrite(enablePin, LOW);
  }
//  Serial.println("--------");
}

void moveBall()
{
  /*
  Serial.print("IN: posn_x ");
  Serial.print(posn_x);
  Serial.print(" posn_y ");
  Serial.print(posn_y);
  Serial.print(" isUp ");
  Serial.print(isUp);
  Serial.print(" isLeft ");
  Serial.println(isLeft);
  */
  if (isUp)
  {
    if (--posn_x <= 0)
    {
      posn_x = 0;
      isUp = false;
    }
  }
  else
  {
    if (++posn_x >= MAX_ROWS)
    {
      posn_x -= 2;
      isUp = true;
    }
  }
  if (isLeft)
  {
    if (--posn_y <= 0)
    {
      posn_y = 0;
      isLeft = false;
    }
  }
  else
  {
    if (++posn_y >= BALL_PARK_WIDTH)
    {
      posn_y -= 2;
      isLeft = true;
    }
  }
  /*
  Serial.print("OUT: posn_x ");
  Serial.print(posn_x);
  Serial.print(" posn_y ");
  Serial.print(posn_y);
  Serial.print(" isUp ");
  Serial.print(isUp);
  Serial.print(" isLeft ");
  Serial.println(isLeft);
  */
}

