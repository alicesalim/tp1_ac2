// Pinos dos LEDs (MSB a LSB: F3, F2, F1, F0)
const int ledPins[4] = {13, 12, 11, 10};

// Memória: 100 posições
byte memoria[100] = {0};

// PC está na posição 0 do vetor
#define PC memoria[0]
#define W  memoria[1]
#define X  memoria[2]
#define Y  memoria[3]

// Programa começa na posição 4
const int inicioPrograma = 4;

void setup() {
  // Configurar pinos dos LEDs como saída
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  // Iniciar comunicação serial
  Serial.begin(9600);
  
  // Inicializar PC
  PC = inicioPrograma;
  
  Serial.println("Sistema ULA 4 bits iniciado");
  Serial.println("Comandos: carregar, executar, dump, reset");
  Serial.println("Formato das instrucoes: XXX (3 digitos hex)");
}

void atualizarLEDs(byte valor) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], (valor >> (3 - i)) & 1);
  }
}

byte ULA(byte A, byte B, byte S) {
  switch (S) {
    case 0x0: return 0xF;                    // umL
    case 0x1: return 0x0;                    // zeroL
    case 0x2: return A | (~B & 0xF);         // A+B'
    case 0x3: return (~A & 0xF) | (~B & 0xF); // A'+B'
    case 0x4: return ~(A & B) & 0xF;         // (A.B)'
    case 0x5: return (~B) & 0xF;             // B'
    case 0x6: return (~A) & 0xF;             // A'
    case 0x7: return ((~A) & 0xF) ^ ((~B) & 0xF); // A'⊕B'
    case 0x8: return A ^ B;                  // A⊕B
    case 0x9: return A;                      // copiaA
    case 0xA: return B;                      // copiaB
    case 0xB: return A & B;                  // A.B
    case 0xC: return A & (~B & 0xF);         // A.B'
    case 0xD: return (~A & 0xF) & B;         // A'.B
    case 0xE: return A | B;                  // A+B
    case 0xF: return ~((~A & 0xF) & B) & 0xF; // (A'.B)'
    default: return 0;
  }
}

void dumpMemoria() {
  Serial.print("->| ");
  // Mostrar PC, W, X, Y
  for (int i = 0; i < 4; i++) {
    if (memoria[i] < 0x10) Serial.print("0");
    Serial.print(memoria[i], HEX);
    Serial.print(" | ");
  }
  
  // Mostrar instruções até a última posição não zero
  bool mostrouAlgo = false;
  for (int i = inicioPrograma; i < 100; i += 2) {
    if (memoria[i] == 0 && memoria[i + 1] == 0 && i > inicioPrograma + 2) break;
    
    // Reconstruir instrução de 3 dígitos hex
    byte XY = memoria[i];
    byte S = memoria[i + 1];
    byte valorX = (XY >> 4) & 0xF;
    byte valorY = XY & 0xF;
    
    // Imprimir como XXX
    Serial.print(valorX, HEX);
    Serial.print(valorY, HEX);
    Serial.print(S, HEX);
    Serial.print(" | ");
    mostrouAlgo = true;
  }
  
  // Se não mostrou nenhuma instrução, mostrar pelo menos as barras
  if (!mostrouAlgo) {
    Serial.print("   | ");
  }
  
  Serial.println();
}

void carregarPrograma() {
  Serial.println("Modo de carga ativado.");
  Serial.println("Digite as instrucoes no formato XXX (3 digitos hex)");
  Serial.println("Exemplo: C6B A3E 123");
  Serial.println("Digite 'fim' para terminar a carga");
  
  int pos = inicioPrograma;
  
  while (pos < 98) { // Para até 98 para caber 2 bytes por instrução
    if (Serial.available() > 0) {
      String linha = Serial.readStringUntil('\n');
      linha.trim();
      
      if (linha.length() == 0) continue;
      
      // Verificar se é comando para sair
      if (linha == "fim" || linha == "FIM") {
        break;
      }
      
      // Remover espaços e processar cada grupo de 3 caracteres
      linha.replace(" ", "");
      for (int i = 0; i < linha.length(); i += 3) {
        if (pos >= 98) {
          Serial.println("Memoria cheia!");
          break;
        }
        
        String grupo = linha.substring(i, i + 3);
        if (grupo.length() < 3) {
          Serial.println("Instrucao incompleta: " + grupo);
          break;
        }
        
        // Converter os caracteres hexadecimais para valores
        char charX = grupo.charAt(0);
        char charY = grupo.charAt(1);
        char charS = grupo.charAt(2);
        
        byte valorX = (charX >= 'A') ? (charX - 'A' + 10) : (charX - '0');
        byte valorY = (charY >= 'A') ? (charY - 'A' + 10) : (charY - '0');
        byte valorS = (charS >= 'A') ? (charS - 'A' + 10) : (charS - '0');
        
        // Armazenar XY no primeiro byte, S no segundo
        memoria[pos] = (valorX << 4) | valorY;
        memoria[pos + 1] = valorS;
        
        pos += 2;
        
        Serial.print("Instrucao ");
        Serial.print(grupo);
        Serial.print(" carregada na posicao ");
        Serial.println(pos - 2);
      }
      
      // Mostrar memória atualizada
      dumpMemoria();
      Serial.println("Digite mais instrucoes ou 'fim' para terminar.");
    }
  }
  
  Serial.println("Carga do programa concluida.");
}

void executarInstrucao() {
  if (PC >= 98) { // 98 porque precisa de 2 bytes por instrução
    Serial.println("Fim do programa - PC fora dos limites");
    return;
  }
  
  byte XY = memoria[PC];
  byte S = memoria[PC + 1];
  
  if (XY == 0 && S == 0) {
    Serial.println("Fim do programa - instrucao zero");
    return;
  }
  
  // Decodificar X e Y
  byte valorX = (XY >> 4) & 0xF;
  byte valorY = XY & 0xF;
  
  // Atualizar X e Y na memória
  X = valorX;
  Y = valorY;
  
  Serial.print("Executando: X=");
  Serial.print(valorX, HEX);
  Serial.print(" Y=");
  Serial.print(valorY, HEX);
  Serial.print(" S=");
  Serial.println(S, HEX);
  
  // Executar ULA
  W = ULA(X, Y, S);
  
  // Atualizar LEDs
  atualizarLEDs(W);
  
  Serial.print("Resultado W=");
  Serial.println(W, HEX);
  
  // Incrementar PC para próxima instrução (2 bytes por instrução)
  PC += 2;
  
  // Mostrar dump da memória
  dumpMemoria();
  
  delay(2000); // Intervalo de 2 segundos entre instruções
}

void executarPrograma() {
  Serial.println("Iniciando execucao do programa...");
  
  while (PC < 98) { // 98 porque precisa de 2 bytes por instrução
    byte XY = memoria[PC];
    byte S = memoria[PC + 1];
    
    // Parar se encontrar instrução zero
    if (XY == 0 && S == 0) {
      Serial.println("Fim do programa - instrucao zero encontrada");
      break;
    }
    
    executarInstrucao();
    
    // Pequena pausa para não sobrecarregar a serial
    delay(100);
  }
  
  Serial.println("Execucao do programa concluida.");
}

void resetarMemoria() {
  for (int i = 0; i < 100; i++) {
    memoria[i] = 0;
  }
  PC = inicioPrograma;
  W = 0;
  X = 0;
  Y = 0;
  atualizarLEDs(0);
  Serial.println("Memoria resetada.");
}

void loop() {
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    
    if (comando == "carregar") {
      carregarPrograma();
    } else if (comando == "executar") {
      executarPrograma();
    } else if (comando == "dump") {
      dumpMemoria();
    } else if (comando == "reset") {
      resetarMemoria();
    } else if (comando == "help" || comando == "?") {
      Serial.println("Comandos disponiveis:");
      Serial.println("carregar - Entrar no modo de carga de programa");
      Serial.println("executar - Executar programa carregado");
      Serial.println("dump     - Mostrar estado atual da memoria");
      Serial.println("reset    - Resetar memoria e PC");
      Serial.println("help     - Mostrar esta ajuda");
    } else {
      Serial.println("Comando desconhecido. Digite 'help' para ajuda.");
    }
  }
}