#include <iostream>
#include <fstream>
#include <string>
using namespace std;

/**
 * Método para verificar se todos os caracteres são dígitos
 * @param s String de entrada
 * @return true se todos os caracteres são dígitos, false caso contrário
 */
bool ehNumero(string s)
{
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] < '0' || s[i] > '9')
        {
            return false;
        }
    }
    return true;
}

/**
 * Método para converter uma string em inteiro
 * @param s String de entrada
 * @return Número inteiro correspondente à string
 */
int stringParaInt(string s)
{
    int num = 0;
    for (size_t i = 0; i < s.size(); i++)
    {
        num = num * 10 + (s[i] - '0'); // transforma caractere em número
    }
    return num;
}

/**
 * Método para converter W em código hexadecimal
 * @param W String de entrada representando a função lógica
 * @return Código hexadecimal correspondente à função
 */
string WtoHex(string W)
{
    if (W == "umL") return "0"; //Função: 1 Lógico
    if (W == "zeroL") return "1"; //Função: 0 Lógico
    if (W == "AonB") return "2"; // Função: A+B'
    if (W == "nAonB") return "3"; //Função: A'+B'
    if (W == "AeBn") return "4"; //Função: (A.B)'
    if (W == "nB") return "5"; //Função: B'
    if (W == "nA") return "6"; //Função: A'
    if (W == "nAxnB") return "7"; //Função A'x B'
    if (W == "AxB") return "8"; //Função AxB
    if (W == "copiaA") return "9"; //Função: A
    if (W == "copiaB") return "A"; //Função: B
    if (W == "AeB") return "B"; //Função: A.B
    if (W == "AenB") return "C"; //Função: A.B'
    if (W == "nAeB") return "D"; //Função: A'.B
    if (W == "AoB") return "E"; //Função: A+B
    if (W == "nAeBn") return "F"; //Função (A'.B)'
    return "";
}

/**
 * Método para converter valores decimais >= 10 em caractere hexadecimal
 * @param valor String de entrada representando o valor decimal
 * @return String com o valor convertido em hexadecimal, se aplicável
 */
string DecToHex(string valor)
{
    if (valor.empty()) return valor;
    if (ehNumero(valor))
    {
        int num = stringParaInt(valor);
        if (num >= 10)
        {
            char hexChar = 'A' + (num - 10);
            return string(1, hexChar);
        }
    }
    return valor;
}

int main()
{
    // Lê do arquivo .ula e escreve no arquivo .hex
    string caminhoEntrada = "C:\\Users\\1472494\\Desktop\\tp1_ac2\\dados\\testeula.ula";
    string caminhoSaida   = "C:\\Users\\1472494\\Desktop\\tp1_ac2\\dados\\testeula.hex";

    // Abre arquivo de entrada (.ula) para leitura
    ifstream entrada(caminhoEntrada);

    // Abre arquivo de saída (.hex) para escrita
    ofstream saida(caminhoSaida);

    if (!entrada.is_open())
    {
        cerr << "Erro ao abrir arquivo de entrada: " << caminhoEntrada << endl;
        return 1;
    }

    if (!saida.is_open())
    {
        cerr << "Erro ao abrir arquivo de saída: " << caminhoSaida << endl;
        entrada.close();
        return 1;
    }

    //cout << "Arquivos abertos com sucesso!" << endl;
    //cout << "Lendo de: " << caminhoEntrada << endl;
    //cout << "Escrevendo em: " << caminhoSaida << endl;

    string linha, X = "", Y = "", W = "";

    // Lê o arquivo .ula linha por linha
    while (getline(entrada, linha))
    {
        // Se encontrar "fim.", interrompe
        if (linha.find("fim.") != string::npos) break;

        // Ignora "inicio"
        if (linha.find("inicio") != string::npos) continue;

        // Remove ponto e vírgula
        size_t posPonto = linha.find(';');
        if (posPonto != string::npos)
        {
            linha = linha.substr(0, posPonto);
        }

        // Remove espaços extras
        while (!linha.empty() && linha.front() == ' ') linha.erase(linha.begin());
        while (!linha.empty() && linha.back() == ' ') linha.pop_back();

        if (linha.empty()) continue;

        // Processa X
        if (linha[0] == 'X')
        {
            size_t pos = linha.find('=');
            if (pos != string::npos)
            {
                X = linha.substr(pos + 1);
                while (!X.empty() && X.front() == ' ') X.erase(X.begin());
                //cout << "X encontrado: " << X << endl;
            }
        }

        // Processa Y
        else if (linha[0] == 'Y')
        {
            size_t pos = linha.find('=');
            if (pos != string::npos)
            {
                Y = linha.substr(pos + 1);
                while (!Y.empty() && Y.front() == ' ') Y.erase(Y.begin());
                //cout << "Y encontrado: " << Y << endl;
            }
        }

        // Processa W
        else if (linha[0] == 'W')
        {
            size_t pos = linha.find('=');
            if (pos != string::npos)
            {
                W = linha.substr(pos + 1);
                while (!W.empty() && W.front() == ' ') W.erase(W.begin());

                // Converte W para hexadecimal
                string Whex = WtoHex(W);
                //cout << "W encontrado: " << W << " -> " << Whex << endl;

                // Converte X e Y para hexadecimal se >= 10
                string Xhex = DecToHex(X);
                string Yhex = DecToHex(Y);

                // Monta a instrução final
                string resultado = Xhex + Yhex + Whex;

                // Escreve no arquivo .hex
                saida << resultado << endl;
                //cout << "Resultado escrito no .hex: " << resultado << endl;
            }
        }
    }

    entrada.close();
    saida.close();
    cout << "Processamento concluido! Arquivo .hex gerado com sucesso." << endl;

    return 0;
}
