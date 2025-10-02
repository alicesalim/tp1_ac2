// ---------------------------
// Configurações do Arduino
// ---------------------------
// Define os pinos digitais conectados aos LEDs.
// Cada LED representa uma saída da ULA (MSB -> LSB: F3, F2, F1, F0).
const int ledPins[4] = {13, 12, 11, 10}; // Ordem fixa: LED mais à esquerda = bit mais significativo (F3) // MSB -> LSB: F3 F2 F1 F0

// ---------------------------
// UI / execução
// ---------------------------
// Variáveis para interface e controle de execução.
// SETA: caractere usado para indicar a instrução atual.
// VERBOSE: controla impressão detalhada no Serial Monitor.
// PRINT_DELAY_MS: tempo de espera (ms) entre execuções para visualização.
const char* SETA = "v";
bool  VERBOSE = false;                 // false = não imprime “Executando/Resultado”
unsigned int PRINT_DELAY_MS = 2000; // Tempo de espera (ms) entre prints das instruções    // tempo de espera após cada instrução

// ---------------------------
// Memória e registradores
// ---------------------------
// Simula a memória principal da CPU.
// Layout: [0]=PC (contador de programa), [1]=W (acumulador), [2]=X, [3]=Y.
// Programa começa no índice 4 (INICIO).
// Demais posições armazenam instruções em formato hexadecimal.
// Layout: [0]=PC, [1]=W, [2]=X, [3]=Y, programa inicia em 4
byte memoria[196] = {0}; // Vetor que representa toda a memória simulada (196 bytes)

#define PC memoria[0] // Contador de programa (aponta para próxima instrução)
#define W  memoria[1] // Acumulador principal (resultado das operações)
#define X  memoria[2] // Registrador auxiliar X
#define Y  memoria[3] // Registrador auxiliar Y

const int INICIO      = 4;
const int MEM_BYTES   = sizeof(memoria) / sizeof(memoria[0]); // 196
const int ULTIMO_BYTE = MEM_BYTES - 1;                        // 195
const int MAX_INSTR   = (MEM_BYTES - INICIO) / 2;             // 96

// ---------------------------
// Estado do programa carregado
// ---------------------------
int  NUM_INSTR = 0;             // quantidade de instruções carregadas
int  END_PC    = INICIO;        // endereço imediatamente após a última instrução
bool PROGRAM_LOADED = false;    // já carregou um programa?
bool HALTED          = false;   // já terminou e travou a execução?

// ---------------------------
// Utilidades
// ---------------------------
inline void printSpaces(int n) { for (int i = 0; i < n; i++) Serial.print(' '); }
inline int myMin(int a, int b){ return (a < b) ? a : b; }
inline int myMax(int a, int b){ return (a > b) ? a : b; }

void atualizarLEDs(byte valor) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], (valor >> (3 - i)) & 1);
  }
}

int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

// ---------------------------
// ULA 4 bits: retorna nibble (0..15)
// ---------------------------
byte ULA(byte A, byte B, byte S) {
  switch (S & 0xF) {
    case 0x0: return 0x1;                       // umL
    case 0x1: return 0x0;                       // zeroL
    case 0x2: return A | (~B & 0xF);            // A + B'
    case 0x3: return (~A & 0xF) | (~B & 0xF);   // A' + B'
    case 0x4: return ~(A & B) & 0xF;            // (A.B)'
    case 0x5: return (~B) & 0xF;                // B'
    case 0x6: return (~A) & 0xF;                // A'
    case 0x7: return ((~A) & 0xF) ^ ((~B) & 0xF); // A' ⊕ B'
    case 0x8: return A ^ B;                     // A ⊕ B
    case 0x9: return A;                         // copiaA
    case 0xA: return B;                         // copiaB
    case 0xB: return A & B;                     // A.B
    case 0xC: return A & (~B & 0xF);            // A.B'
    case 0xD: return (~A & 0xF) & B;            // A'.B
    case 0xE: return A | B;                     // A + B
    case 0xF: return ~((~A & 0xF) & B) & 0xF;   // (A'.B)'
    default:  return 0;
  }
}

// ---------------------------
// Dump com seta vertical
// ---------------------------
void dumpMemoria() {
  const int colInicio    = 4;
  const int larguraReg   = 5;                          // "XX | "
  const int baseInstrCol = colInicio + 4 * larguraReg; // 24
  const int colW         = colInicio + 1 * larguraReg + 1;

  // Calcular PC sequencial em DECIMAL (4,5,6,7,8...)
  int PC_sequencial = INICIO + (PC - INICIO) / 2;
  
  // seta sobre a instrução apontada por PC (apenas se dentro do programa)
  int deslocInstrAntesPC = 0;
  bool temInstrucao = (PC >= INICIO && PC < END_PC);

  // MODIFICAÇÃO: Deslocamento FIXO entre instruções
  for (int i = INICIO; temInstrucao && i < END_PC; i += 2) {
    if (i == PC) break;
    // Deslocamento FIXO de 6 espaços por instrução
    deslocInstrAntesPC += 6;
  }
  
  // MODIFICAÇÃO: Posicionar a seta no meio da instrução
  const int colInstr = temInstrucao ? (baseInstrCol + deslocInstrAntesPC + 1) : -1;

  // linha das setas
  if (temInstrucao) {
    int c1 = myMin(colW, colInstr);
    int c2 = myMax(colW, colInstr);
    printSpaces(c1); Serial.print(SETA);
    printSpaces(c2 - c1 - 1); Serial.print(SETA);
    Serial.println();
  } else {
    printSpaces(colW); Serial.print(SETA); Serial.println();
  }

  // Mostrar PC sequencial em DECIMAL
  Serial.print("->| ");
  // Mostra PC sequencial em DECIMAL
  if (PC_sequencial < 10) Serial.print('0');
  Serial.print(PC_sequencial);  // SEM hexadecimal - mostra em decimal
  Serial.print(" | ");
  
  // Mostra os outros registradores em HEXADECIMAL (como antes)
  for (int i = 1; i < 4; i++) {
    if (memoria[i] < 0x10) Serial.print('0');
    Serial.print(memoria[i], HEX);
    Serial.print(" | ");
  }

  bool mostrouAlgo = false;
  
  // Mostra as instruções sem números de posição
  for (int i = INICIO; i < END_PC; i += 2) {
    byte XY = memoria[i];
    byte S  = memoria[i + 1];
    byte valorX = (XY >> 4) & 0xF;
    byte valorY = XY & 0xF;

    Serial.print(valorX, HEX);
    Serial.print(valorY, HEX);
    Serial.print(S, HEX);
    Serial.print(" | ");
    mostrouAlgo = true;
  }
  if (!mostrouAlgo) Serial.print(" | ");
  Serial.println();
}
// ---------------------------
// Carregar programa (sem exigir "fim")
// Lê tudo que estiver disponível na linha, conta instruções e define END_PC.
// ---------------------------
void carregarPrograma() {
  // zera área de programa e registradores
  for (int i = INICIO; i < MEM_BYTES; i++) memoria[i] = 0;
  PC = INICIO; W = 0; X = 0; Y = 0;

  NUM_INSTR = 0;
  END_PC    = INICIO;
  PROGRAM_LOADED = false;

  // aguarda ter algo no buffer (se quiser enviar tudo de uma vez na mesma linha)
  while (Serial.available() == 0) { delay(2); }

  // lê uma linha inteira com as instruções (ex.: "A10 B21 C32 ...")
  String linha = Serial.readStringUntil('\n');
  linha.trim();
  if (linha.length() == 0) {
    Serial.println("Linha vazia. Nada carregado.");
    return;
  }

  // aceita "C6BA3E123" ou "C6B A3E 123"
  linha.replace(" ", "");

  int pos = INICIO;

  for (int i = 0; i + 2 < linha.length(); i += 3) {
    if ((pos + 1) > ULTIMO_BYTE) {
      Serial.println("Memoria cheia. Instrucoes extras foram ignoradas.");
      break;
    }

    char charX = linha.charAt(i);
    char charY = linha.charAt(i + 1);
    char charS = linha.charAt(i + 2);

    int nx = hexNibble(charX), ny = hexNibble(charY), ns = hexNibble(charS);
    if (nx < 0 || ny < 0 || ns < 0) {
      // token inválido: para de carregar a partir daqui
      Serial.print("Token invalido ignorado (parada no carregamento): ");
      Serial.print(charX); Serial.print(charY); Serial.println(charS);
      break;
    }

    byte valorX = (byte)nx & 0xF;
    byte valorY = (byte)ny & 0xF;
    byte valorS = (byte)ns & 0xF;

    memoria[pos]     = (valorX << 4) | valorY; // XY
    memoria[pos + 1] = valorS;                 // S
    pos += 2;
    NUM_INSTR++;

    if (NUM_INSTR >= MAX_INSTR) {
      //Serial.println("Atingiu MAX_INSTR. Restante ignorado.");
      break;
    }
  }

  END_PC = INICIO + (2 * NUM_INSTR);
  PROGRAM_LOADED = (NUM_INSTR > 0);

  dumpMemoria();

}

// ---------------------------
// Execução: respeita END_PC
// ---------------------------
void executarInstrucao() {
  if (PC >= END_PC) { // nada a executar
    return;
  }

  byte XY = memoria[PC];
  byte S  = memoria[PC + 1];

  // Decodifica X e Y
  byte valorX = (XY >> 4) & 0xF;
  byte valorY = XY & 0xF;

  // Atualiza registradores
  X = valorX;
  Y = valorY;

  if (VERBOSE) {
    Serial.print("Executando: X="); Serial.print(valorX, HEX);
    Serial.print(" Y=");            Serial.print(valorY, HEX);
    Serial.print(" S=");            Serial.println(S, HEX);
  }

  // ULA
  W = ULA(X, Y, S);

  // LEDs
  atualizarLEDs(W);

  if (VERBOSE) {
    Serial.print("Resultado W="); Serial.println(W, HEX);
  }

  // Próxima instrução
  PC += 2;

  // Dump após executar
  dumpMemoria();

  delay(PRINT_DELAY_MS);
}

void executarPrograma() {
  if (!PROGRAM_LOADED || NUM_INSTR == 0) {
    Serial.println("Nenhum programa carregado.");
    return;
  }

  while (PC < END_PC) {      // <-- executa SOMENTE as instruções carregadas
    executarInstrucao();
    delay(100); // pequena pausa
  }

  // terminou o programa
  delay(2000);
  HALTED = true;             // trava a loop() (nao executa de novo)
}

// ---------------------------
// Reset
// ---------------------------
void resetarMemoria() {
  for (int i = 0; i < MEM_BYTES; i++) memoria[i] = 0;
  PC = INICIO; W = 0; X = 0; Y = 0;
  NUM_INSTR = 0;
  END_PC    = INICIO;
  PROGRAM_LOADED = false;
  HALTED = false;
  atualizarLEDs(0);
}

// ---------------------------
// Arduino setup/loop
// ---------------------------
void setup() {
  for (int i = 0; i < 4; i++) pinMode(ledPins[i], OUTPUT);
  Serial.begin(9600);
  resetarMemoria();
}

void loop() {
  if (HALTED) {
    // Programa finalizado — não faça mais nada.
    // (Se quiser aceitar novo programa sem reset físico,
    //  troque HALTED por PROGRAM_LOADED=false quando detectar dados.)
    return;
  }

  // Carrega e executa UMA VEZ o que chegar na próxima linha:
  if (!PROGRAM_LOADED && Serial.available() > 0) {
    carregarPrograma();   // lê a linha e define NUM_INSTR/END_PC
    executarPrograma();   // roda só até END_PC e para
  }

  // Se quiser aceitar novo programa automaticamente depois de terminar,
  // comente HALTED=true lá em executarPrograma() e aqui coloque:
  // if (!PROGRAM_LOADED && Serial.available() > 0) { ... }
}
