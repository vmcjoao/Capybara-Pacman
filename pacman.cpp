// --- Bibliotecas Utilizadas ---
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <optional>
#include <queue>
#include <random>

using namespace std;

// ===================================================================
// CONFIGURAÇÕES GLOBAIS E CONSTANTES
// ===================================================================
const int SIZE = 50; // Define o tamanho, em pixels, de cada célula do grid do mapa
const int LAR = 25;  // Largura do mapa, medida em número de células (SIZE)
const int ALT = 13;  // Altura do mapa, também em número de células
const int MAP_WIDTH = LAR * SIZE;
const int MAP_HEIGHT = ALT * SIZE;
const int UI_PANEL_WIDTH = 50;
const int UI_BAR_HEIGHT = 50;
const int WINDOW_WIDTH = MAP_WIDTH + UI_PANEL_WIDTH;
const int WINDOW_HEIGHT = MAP_HEIGHT + UI_BAR_HEIGHT;
enum DIRECOES {      // Enum para representar as direções
    CIM,
    DIR,
    BAI,
    ESQ
};
enum GameState {     // Controla a máquina de estados do jogo, determinando o que está acontecendo (Menu, Jogo, etc.)
    MENU,
    ESCOLHA_DIFICULDADE,
    INICIANDO, 
    JOGANDO,
    PAUSADO,
    CONFIGURACOES,
    GAME_OVER,
    COMENDO_CROC,
    TELA_FIM_DE_JOGO,
    VITORIA
};
const float delayNormal = 0.25f;       // Tempo (em segundos) que o jogador leva para se mover uma célula em terra
const float delayLento = 0.35f;        // Tempo que o jogador leva para se mover uma célula na água (penalidade)
const float FRAME_DELAY = 0.07f;       // Duração de cada frame na animação da tela de loading
const float LOADING_DURATION = FRAME_DELAY * 22; // Duração total da tela de loading, calculada a partir dos frames
const float WATERMELON_RUSH_DURATION = 8.f;
const float CAP_EAT_CROC_FRAME_DELAY = 0.025f;
const int CAP_EAT_CROC_FRAME_COUNT = 8;
const int CROCODILE_BONUS = 500;
const string HIGHSCORE_FILE = "highscore.txt";
float delayAnim = 0.2f;                // Velocidade da interpolação visual (movimento suave entre células)
int pontuacao;                         // Armazena a pontuação da partida atual
int numPontos = 0;                     // Contador total de folhas no mapa, usado para checar a condição de vitória
int ppf = 50;                          // Pontos Por Folha: quantos pontos o jogador ganha ao coletar uma folha
bool stop = false;                     // Flag para "congelar" a lógica de movimento, útil durante animações de transição
vector<int> Bonus;                     // Talvez para futuros power-ups
vector<bool> Life = { 1, 1, 1, 1, 1 }; // Representa as vidas do jogador
int dist[ALT][LAR];                    // Matriz que armazena as distâncias do jogador para cada célula, usada pela IA (BFS)
const int dx[4] = { 0, 1, 0, -1 };     // Vetores para facilitar o cálculo de movimento no eixo X
const int dy[4] = { -1, 0, 1, 0 };     // Vetores para facilitar o cálculo de movimento no eixo Y
sf::Font font;                         // Fonte padrão usada nos textos do jogo
sf::Text scoreText{font};              // Texto da SFML para exibir a pontuação
sf::Texture placeholderTexture;        // Textura vazia para construir sprites antes do carregamento real
vector<sf::Texture> Loading;           // Armazena os frames da animação de loading
vector<sf::Texture> GameOver[4];       // Armazena os frames da animação de morte, um set para cada direção
vector<sf::Texture> CapEatCroc[4];     // Armazena os frames da capivara comendo crocodilo
vector<sf::Texture> animacaoVitoria;   // Armazena os frames da animação de vitória
int dificuldadeJogo = 3;               // Guarda a dificuldade dos fantasmas
vector<bool> melan = { 0, 0 };         // Representa as vidas extras, conseguidas com powerups

// ===================================================================
// STRUCTS
// ===================================================================

// --- Struct para o Jogador ---
struct Player {
    int potuacaoTotal = 0; // Pontuação total entre partidas

    // --- Posição e Lógica de Movimento ---
    int posx = 12, posy = 7;                                     // Posição no grid do mapa (coordenadas lógicas)
    bool cima = false, baixo = false, esq = false, dir = false;  // Flags que indicam a direção do movimento contínuo
    int wait = -1;                                               // "Buffer" de input: guarda a última tecla pressionada para ser executada na próxima célula válida
    int ultimadir = DIR;                                         // Guarda a última direção de movimento para manter o sprite virado para o lado certo quando parado
    bool dead = false;                                           // Indica se o jogador perdeu todas as vidas
    int prev_posx = 12;                                          // Posição anterior, usada para detectar colisão cruzada
    int prev_posy = 7;

    // --- Animação e Gráficos ---
    sf::Vector2f visualPos; // Posição na tela (em pixels), usada para a interpolação que cria o movimento suave
    sf::Sprite Sprite{placeholderTexture}; // Sprite do jogador
    int animationFrame = 0; // Índice do frame atual na sequência de animação
};

// --- Struct para os Fantasmas ---
struct Ghost {
    // --- Posição e Lógica de Movimento ---
    int x = 11, y = 11;      // Posição lógica atual do fantasma no mapa
    int xinicial, yinicial;  // Posição inicial para onde ele retorna após uma colisão
    int dir = CIM;           // Direção atual do movimento (usando o enum DIRECOES)
    int dificuldade = 5;     // Nível de "inteligência": chance (em 10) de seguir o caminho mais curto até o jogador
    bool isWeak = false;     // (Não utilizado no código atual) - Seria usado para um estado "assustado"
    bool release = false;    // Flag que controla se o fantasma já saiu da sua base inicial
    bool eating = false;     // Flag para indicar se este fantasma está na animação de "comer" o jogador

    // --- Animação e Gráficos ---
    sf::Vector2f visualPos;         // Posição visual em pixels, para o movimento suave
    sf::Sprite sprite{placeholderTexture}; // Sprite do fantasma
    vector<sf::Texture> animNormal[4]; // Vetores de texturas para animação em terra (um para cada direção)
    vector<sf::Texture> animSwim[4];   // Vetores de texturas para animação na água
    vector<sf::Texture> animWeak[4];   // Vetores de texturas para o modo melancia em terra
    vector<sf::Texture> animWeakSwim[4]; // Vetores de texturas para o modo melancia na água
    int animFrame = 0;              // Índice do frame atual da animação

    // --- Controle de Velocidade ---
    const float delayNormal = 0.25f; // Velocidade padrão do fantasma
    const float delayAgua = 0.18f;   // Velocidade do fantasma na água (mais rápido que o jogador na água)
    float delayAnim = 0.25f;         // Velocidade da interpolação visual
    float delay;                     // Velocidade atual, que muda dependendo do terreno
    float releaseTime;               // Tempo (em segundos) que o fantasma espera na base antes de ser liberado
    sf::Clock Clock;                 // Relógio individual para controlar o intervalo de movimento deste fantasma
    sf::Clock releaseClock;          // Relógio para contar o tempo de espera antes da liberação
};

// ===================================================================
// DEFINIÇÃO DO MAPA
// '1': Parede
// '0'/'3'/'4': Caminho livre
// '2': Água
// ===================================================================
char mapa[ALT][LAR + 1] = {
    "1111111011111111011111111",
    "1300031303000030313000031",
    "1011101110111101110111101",
    "1304030003000000003030031",
    "1011111110111111110101101",
    "1300030003022200003030031",
    "1011101111122211111101101",
    "0300033000022200003030030",
    "1011110111111111110111101",
    "1300003300033000303000031",
    "1011111011100111011111101",
    "1300000312222221300400031",
    "1111111011111111011111111" };
// Mapa auxiliar, apenas para marcar onde ainda existem folhas a serem coletadas
bool mapaFolhas[ALT][LAR] = { false };

// --- Variáveis Globais de Controle de Tempo ---
sf::Clock deltaClock;                  // Mede o tempo entre cada frame (Delta Time)
sf::Clock animationClock;              // Controla a frequência com que os frames dos sprites são trocados
const float ANIMATION_SPEED = 0.15f;   // Intervalo (em segundos) para a troca dos frames de animação
sf::Clock clockVitoria;                // Clock da animacao de vitória

// --- Vetores Globais para guardar as texturas de animação do jogador ---
vector<sf::Texture> Texturas[4];     // Animações do jogador em terra
vector<sf::Texture> Texturasswim[4]; // Animações do jogador na água

// ===================================================================
// FUNÇÕES AUXILIARES
// ===================================================================

// --- Funções de Carregamento de Animação ---
// Carrega uma sequência de frames para a animação do jogador
void Animacaopacman(int dir, const string& prefixo, int frameCount) {
    for (int i = 0; i < frameCount; ++i) {
        sf::Texture tex;
        string nomeArquivo = "assets/pacman-" + prefixo + "-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo imagem " << nomeArquivo << "\n";
        else
            cout << "Sprite " << nomeArquivo << " carregado com sucesso" << endl;
        Texturas[dir].push_back(tex);
    }
}
// Carrega a sequência de frames do jogador nadando
void Animacaoswimpacman(int dir, const string& prefixo, int frameCount) {
    for (int i = 0; i < frameCount; ++i) {
        sf::Texture tex;
        string nomeArquivo = "assets/pacman-swim-" + prefixo + "-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo imagem " << nomeArquivo << "\n";
        else
            cout << "Sprite " << nomeArquivo << " carregado com sucesso" << endl;
        Texturasswim[dir].push_back(tex);
    }
}
// Carrega uma sequência de frames para a animação de um fantasma
void AnimacaoFantasma(Ghost& g, int dir, const string& prefixo, const string& nome, int frameCount) {
    for (int i = 0; i < frameCount; ++i) {
        sf::Texture tex;
        string nomeArquivo = "assets/" + nome + "-" + prefixo + "-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo " << nomeArquivo << "\n";
        else
            cout << "Sprite " << nomeArquivo << " carregado com sucesso" << endl;
        g.animNormal[dir].push_back(tex);
    }
}
// Carrega a sequência de frames de um fantasma nadando
void AnimacaoFantasmaSwim(Ghost& g, int dir, const string& prefixo, const string& nome, int frameCount) {
    for (int i = 0; i < frameCount; ++i) {
        sf::Texture tex;
        string nomeArquivo = "assets/" + nome + "-swim-" + prefixo + "-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo " << nomeArquivo << "\n";
        else
            cout << "Sprite " << nomeArquivo << " carregado com sucesso" << endl;
        g.animSwim[dir].push_back(tex);
    }
}
// Carrega a sequência de frames de um fantasma vulnerável
void AnimacaoFantasmaWeak(Ghost& g, int dir, const string& prefixo, int frameCount) {
    for (int i = 0; i < frameCount; ++i) {
        sf::Texture tex;
        string nomeArquivo = "assets/croc-weak-" + prefixo + "-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo " << nomeArquivo << "\n";
        else
            cout << "Sprite " << nomeArquivo << " carregado com sucesso" << endl;
        g.animWeak[dir].push_back(tex);
    }
}
// Carrega a sequência de frames de um fantasma vulnerável nadando
void AnimacaoFantasmaWeakSwim(Ghost& g, int dir, const string& prefixo, int frameCount) {
    for (int i = 0; i < frameCount; ++i) {
        sf::Texture tex;
        string nomeArquivo = "assets/croc-weak-swim-" + prefixo + "-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo " << nomeArquivo << "\n";
        else
            cout << "Sprite " << nomeArquivo << " carregado com sucesso" << endl;
        g.animWeakSwim[dir].push_back(tex);
    }
}
// Carrega a animação de "game over" (fantasma comendo o jogador)
void AnimacaoGameOver(int dir, const string& prefixo, int frameCount) {
    for (int i = 0; i < frameCount; ++i) {
        sf::Texture tex;
        string nomeArquivo = "assets/croc-eat-cap-" + prefixo + "-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo imagem " << nomeArquivo << "\n";
        else
            cout << "Sprite " << nomeArquivo << " carregado com sucesso" << endl;
        GameOver[dir].push_back(tex);
    }
}
// Carrega a animação de "capivara comendo crocodilo"
void AnimacaoCapEatCroc(int dir, const string& prefixo, int frameCount) {
    for (int i = 0; i < frameCount; ++i) {
        sf::Texture tex;
        string nomeArquivo = "assets/cap-eat-croc-" + prefixo + "-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo imagem " << nomeArquivo << "\n";
        else
            cout << "Sprite " << nomeArquivo << " carregado com sucesso" << endl;
        CapEatCroc[dir].push_back(tex);
    }
}
// Carrega os frames da tela de loading
void carregarLoading() {
    for (int i = 0; i < 22; ++i) {
        sf::Texture tex;
        string nomeArquivo = "assets/loading-page-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo " << nomeArquivo << "\n";
        Loading.push_back(tex);
    }
    if (Loading.size() == 22) {
        cout << "Tela de loading carregada com sucesso" << endl;
    }
}

// Função para carregar texturas de chão
void carregarTexturaChao(map<string, sf::Texture>& mapaDeTexturas, const string& nomeArquivo) {
    sf::Texture tex;
    if (!tex.loadFromFile("assets/" + nomeArquivo + ".png"))
        cout << "Erro ao carregar a textura do chao: " << nomeArquivo << ".png" << endl;
    else {
        mapaDeTexturas[nomeArquivo] = tex;
        cout << "Textura " << nomeArquivo << " carregado com sucesso" << endl;
    }
}
// Função para carregar outros assets visuais (folha, coração)
void carregarAssets(map<string, sf::Texture>& mapaDeAssets, const string& nomeArquivo) {
    sf::Texture asset;
    if (!asset.loadFromFile("assets/" + nomeArquivo + ".png"))
        cout << "Erro ao carregar o asset: " << nomeArquivo << ".png" << endl;
    else {
        mapaDeAssets[nomeArquivo] = asset;
        cout << "Asset " << nomeArquivo << " carregado com sucesso" << endl;
    }
}
// Função para carregar animação de vitória
void carregarAnimacaoVitoria() {
    for (int i = 0; i < 191; ++i) { // Loop para os 91 quadros
        sf::Texture tex;
        string nomeArquivo = "assets/vitoria/vitframe-" + to_string(i) + ".png";
        if (!tex.loadFromFile(nomeArquivo))
            cout << "Erro lendo " << nomeArquivo << "\n";
        animacaoVitoria.push_back(tex);
    }
    if (animacaoVitoria.size() == 191) {
        cout << "Animacao de vitoria carregada com sucesso!" << endl;
    }
}

// Função para carregar arquivos de áudio em buffers de som
void carregarAudios(map<string, sf::SoundBuffer>& mapaDeAudios, const string& nomeArquivo, string tipo) {
    sf::SoundBuffer sound;
    if (!sound.loadFromFile("assets/Sounds/" + nomeArquivo + tipo))
        cout << "Erro ao carregar o audio: " << nomeArquivo << tipo << endl;
    else {
        mapaDeAudios[nomeArquivo] = sound;
        cout << "Audio " << nomeArquivo << " carregado com sucesso" << endl;
    }
}

class SoundHandle {
public:
    SoundHandle() = default;
    explicit SoundHandle(const sf::SoundBuffer& buffer) : sound(buffer) {}

    void setBuffer(const sf::SoundBuffer& buffer) { sound.emplace(buffer); }
    void setVolume(float volume) { if (sound) sound->setVolume(volume); }
    void play() { if (sound) sound->play(); }
    void stop() { if (sound) sound->stop(); }
    sf::Sound::Status getStatus() const {
        return sound ? sound->getStatus() : sf::Sound::Status::Stopped;
    }

private:
    optional<sf::Sound> sound;
};

class GameResources {
public:
    map<string, sf::Texture> floorTextures;
    map<string, sf::Texture> uiTextures;
    map<string, sf::SoundBuffer> soundBuffers;
    map<string, SoundHandle> sounds;

    void buildSounds() {
        for (auto& soundBuffer : soundBuffers) {
            sounds[soundBuffer.first].setBuffer(soundBuffer.second);
            sounds[soundBuffer.first].setVolume(100.f);
        }
    }
};

// Altera o volume de tudo
void setVolumeGeral(float volume, sf::Music& musicaMenu, map<string, SoundHandle>& Audios) {
    musicaMenu.setVolume(volume);
    for (auto& par : Audios) {
        par.second.setVolume(volume);
    }
    Audios["button-hover"].setVolume(volume / 2);
    Audios["bg-iniciando"].setVolume(volume / 2);
}

template <typename DrawableWithBounds>
void centralizarOrigem(DrawableWithBounds& drawable) {
    sf::FloatRect bounds = drawable.getLocalBounds();
    drawable.setOrigin(sf::Vector2f(
        bounds.position.x + bounds.size.x / 2.0f,
        bounds.position.y + bounds.size.y / 2.0f
    ));
}

sf::Text criarTextoCentralizado(const sf::Font& fonte, const string& texto, unsigned int tamanho, sf::Vector2f posicao) {
    sf::Text text(fonte, texto, tamanho);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color::Black);
    text.setOutlineThickness(2);
    centralizarOrigem(text);
    text.setPosition(posicao);
    return text;
}

void posicionarPonteiro(sf::CircleShape& pointer, const sf::Text& texto) {
    float pointerX = texto.getGlobalBounds().position.x - pointer.getRadius() - 15;
    float pointerY = texto.getPosition().y - 12;
    pointer.setPosition(sf::Vector2f(pointerX, pointerY));
}

void aplicarViewProporcional(sf::RenderWindow& window, sf::View& view) {
    const sf::Vector2u windowSize = window.getSize();
    const float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    const float gameRatio = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);

    float viewportWidth = 1.f;
    float viewportHeight = 1.f;
    float viewportLeft = 0.f;
    float viewportTop = 0.f;

    if (windowRatio > gameRatio) {
        viewportWidth = gameRatio / windowRatio;
        viewportLeft = (1.f - viewportWidth) / 2.f;
    }
    else {
        viewportHeight = windowRatio / gameRatio;
        viewportTop = (1.f - viewportHeight) / 2.f;
    }

    view.setViewport(sf::FloatRect(
        sf::Vector2f(viewportLeft, viewportTop),
        sf::Vector2f(viewportWidth, viewportHeight)
    ));
    window.setView(view);
}

sf::Vector2u calcularTamanhoInicialJanela() {
    const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    const float maxWidth = static_cast<float>(desktop.size.x) * 0.9f;
    const float maxHeight = static_cast<float>(desktop.size.y) * 0.9f;
    const float scale = std::min({
        1.f,
        maxWidth / static_cast<float>(WINDOW_WIDTH),
        maxHeight / static_cast<float>(WINDOW_HEIGHT)
    });

    return sf::Vector2u(
        static_cast<unsigned int>(WINDOW_WIDTH * scale),
        static_cast<unsigned int>(WINDOW_HEIGHT * scale)
    );
}

int carregarRecorde() {
    ifstream arquivo(HIGHSCORE_FILE);
    int recorde = 0;
    if (arquivo >> recorde) {
        return recorde;
    }
    return 0;
}

void salvarRecorde(int recorde) {
    ofstream arquivo(HIGHSCORE_FILE);
    if (arquivo) {
        arquivo << recorde << '\n';
    }
}

void atualizarRecorde(int pontuacaoAtual, int& recorde) {
    if (pontuacaoAtual > recorde) {
        recorde = pontuacaoAtual;
        salvarRecorde(recorde);
    }
}

int indiceMapa(float coordenada, int limite) {
    int indice = static_cast<int>(coordenada + SIZE / 2) / SIZE;
    if (indice < 0) return limite - 1;
    if (indice >= limite) return 0;
    return indice;
}

bool estaNaAgua(sf::Vector2f visualPos) {
    return mapa[indiceMapa(visualPos.y, ALT)][indiceMapa(visualPos.x, LAR)] == '2';
}

void mandarFantasmaParaBase(Ghost& croc) {
    croc.x = croc.xinicial;
    croc.y = croc.yinicial;
    croc.visualPos = sf::Vector2f(croc.x * SIZE, croc.y * SIZE);
    croc.dir = CIM;
    croc.release = false;
    croc.releaseClock.restart();
}

sf::Vector2f offsetCapivaraNaAnimacaoCroc(int dir) {
    if (dir == DIR) return sf::Vector2f(18.f, 6.f);
    if (dir == ESQ) return sf::Vector2f(60.f, 6.f);
    if (dir == CIM) return sf::Vector2f(39.f, 10.f);
    if (dir == BAI) return sf::Vector2f(39.f, 2.f);
    return sf::Vector2f(0.f, 0.f);
}

// --- Funções de Lógica do Jogo ---
// Reseta o estado do jogo, para uma nova partida ou após perder uma vida
void Reset(Player& pacman, vector<Ghost>& ListaFantasmas, bool rstpts) {
    // Recoloca o jogador na posição inicial.
    pacman.posx = 12;
    pacman.posy = 7;
    pacman.dir = pacman.esq = pacman.cima = pacman.baixo = false;
    pacman.wait = -1;
    stop = true; // Ativa a flag para pausar o jogo até a animação de transição terminar

    // Se rstpts (resetar pontos) for verdadeiro, começa novo jogo
    if (rstpts || pacman.dead) {
        numPontos = 0;
        // Preenche o mapa de folhas com base no layout original
        for (int i = 0; i < ALT; ++i) {
            for (int j = 0; j < LAR - 1; ++j) {
                mapaFolhas[i][j] = (mapa[i][j] == '0' || mapa[i][j] == '3' || mapa[i][j] == '4');
                if (mapaFolhas[i][j])
                    numPontos++;
            }
        }
        Life = { 1, 1, 1, 1, 1 }; // Restaura todas as vidas
        melan = { 0, 0 }; // Reseta powerups
        cout << "Vidas restauradas" << endl;
    }
    // Caso contrário, o jogador apenas perdeu uma vida
    else {
        if (!melan[1]) {
            if (!melan[0]) {
                int vidasrestantes = 4;
                // Encontra a última vida e a define como perdida
                for (int i = 4; i >= 0; i--) {
                    if (Life[i] == 1) {
                        Life[i] = 0;
                        cout << "Vidas restantes: " << vidasrestantes << endl;
                        break;
                    }
                    else {
                        vidasrestantes--;
                    }
                }
                // Se todas as vidas acabaram, marca o jogador como morto
                if (vidasrestantes == 0) {
                    pacman.dead = true;
                    cout << "Fim de jogo" << endl;
                }
            }
            else
                melan[0] = 0;
        }
        else
            melan[1] = 0;
    }

    // Reseta todos os fantasmas
    for (auto& croc : ListaFantasmas) {
        croc.x = croc.xinicial;
        croc.y = croc.yinicial;
        croc.dir = CIM;
        croc.eating = false;

        if (&croc == &ListaFantasmas[0]) croc.dificuldade = min(10, dificuldadeJogo * 2);      // Mais difícil
        else if (&croc == &ListaFantasmas[1]) croc.dificuldade = min(10, dificuldadeJogo + 3); // Mediano
        else if (&croc == &ListaFantasmas[2]) croc.dificuldade = min(10, dificuldadeJogo + 1); // Fácil
        else croc.dificuldade = min(10, dificuldadeJogo); // Mais fácil
    }
    for (int i = 0; i < ListaFantasmas.size(); i++) {
        cout << "Dificuldade fantasma " << i + 1 << ": " << "Nivel " << ListaFantasmas[i].dificuldade << endl;
    }
    cout << "Fim do Reset" << endl;
}


// --- Funções de IA dos Fantasmas ---

// Usa o algoritmo Breadth-First Search (BFS) para calcular a menor distância de cada célula do mapa até o jogador
// O resultado é um "mapa de calor" que a IA consulta para encontrar o caminho mais curto
void calcularDistancias(Player& jogador, char mapa[ALT][LAR + 1], int dist[ALT][LAR]) {
    queue<pair<int, int>> fila;

    // 1. Inicializa todas as distâncias como um valor alto (simulando infinito)
    for (int y = 0; y < ALT; ++y)
        for (int x = 0; x < LAR; ++x)
            dist[y][x] = 9999;

    // 2. O ponto de partida é a posição do jogador, com distância 0
    dist[jogador.posy][jogador.posx] = 0;
    fila.push({ jogador.posy, jogador.posx });

    // 3. Processa a fila até que todas as células alcançáveis tenham sido visitadas
    while (!fila.empty()) {
        auto atual = fila.front();
        fila.pop();
        int y = atual.first;
        int x = atual.second;

        // 4. Para cada célula, explora seus 4 vizinhos
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i]; // Para poupar o trabalho de usar varios if else, dx é -1 para esq e 1 para a direita
            int ny = y + dy[i]; // Semelhante ao de cima, mas para y

            // Tratamento do teleporte nas bordas do mapa
            if (nx < 0) nx = LAR - 1;
            else if (nx >= LAR) nx = 0;
            if (ny < 0) ny = ALT - 1;
            else if (ny >= ALT) ny = 0;

            // 5. Se o vizinho não for uma parede e um caminho mais curto for encontrado
            if (mapa[ny][nx] != '1' && dist[ny][nx] > dist[y][x] + 1) {
                dist[ny][nx] = dist[y][x] + 1; // atualiza a distância
                fila.push({ ny, nx });         // e adiciona o vizinho à fila para ser explorado
            }
        }
    }
}
// Função que retorna a direção oposta à recebida
int oposta(int d) {
    if (d == CIM) return BAI;
    if (d == BAI) return CIM;
    if (d == ESQ) return DIR;
    if (d == DIR) return ESQ;
    cout << "Erro na escolha da direcao" << endl;
    return -1;
}
// Função principal da IA. Decide a próxima direção do fantasma com base na dificuldade
int melhorDirecao(Ghost& g, int dist[ALT][LAR], char mapa[ALT][LAR + 1], mt19937& rng) {
    vector<pair<int, int>> opcoesValidas;
    int melhorDistGlobal = 9999;
    int melhorDirGlobal = g.dir;

    // Passo 1: Avaliar todas as direções possíveis a partir da posição atual do fantasma
    for (int i = 0; i < 4; ++i) {
        int nx = g.x + dx[i];
        int ny = g.y + dy[i];
        int dest_x = nx, dest_y = ny;

        // Simula o teleporte para consultar a distância correta na matriz 'dist'
        if (dest_x < 0) dest_x = LAR - 1;
        else if (dest_x >= LAR) dest_x = 0;
        if (dest_y < 0) dest_y = ALT - 1;
        else if (dest_y >= ALT) dest_y = 0;

        // Se o movimento não leva a uma parede
        if (mapa[dest_y][dest_x] != '1') {
            int distanciaAtual = dist[dest_y][dest_x];
            opcoesValidas.push_back({ i, distanciaAtual }); // Adiciona à lista de movimentos possíveis

            // E se essa direção leva a uma célula mais próxima do jogador
            if (distanciaAtual < melhorDistGlobal) {
                melhorDistGlobal = distanciaAtual; // ...ela se torna a nova melhor opção
                melhorDirGlobal = i;
            }
        }
    }

    // Passo 2: Lógica de dificuldade para decidir o movimento
    uniform_int_distribution<int> dist10(1, 10);
    int chance = dist10(rng); // Gera um número aleatório de 1 a 10

    // Se a 'chance' for menor ou igual à dificuldade do fantasma, ele age de forma inteligente
    if (chance <= g.dificuldade) {
        // Lógica para evitar que o fantasma inverta a direção abruptamente em um corredor
        bool melhorOpcaoEhReversa = (melhorDirGlobal == oposta(g.dir));
        if (melhorOpcaoEhReversa && opcoesValidas.size() > 1) {
            // Se a melhor opção é voltar, mas existem outros caminhos, ele procura a segunda melhor opção
            int segundaMelhorDist = 9999;
            int segundaMelhorDir = melhorDirGlobal;
            for (auto& opt : opcoesValidas) {
                if (opt.first != oposta(g.dir) && opt.second < segundaMelhorDist) {
                    segundaMelhorDist = opt.second;
                    segundaMelhorDir = opt.first;
                }
            }
            return segundaMelhorDir; // E segue por ela
        }
        return melhorDirGlobal; // Caso contrário, segue o caminho mais curto
    }
    // Se não, o fantasma se move de forma aleatória (mas ainda para um caminho válido)
    else {
        // Remove a opção de inverter a direção
        vector<pair<int, int>> opcoesSemRe;
        for (auto& opt : opcoesValidas) {
            if (opt.first != oposta(g.dir)) {
                opcoesSemRe.push_back(opt);
            }
        }
        // Se houver opções além de dar ré, escolhe uma delas aleatoriamente
        if (!opcoesSemRe.empty()) {
            uniform_int_distribution<int> distAleatoria(0, opcoesSemRe.size() - 1);
            int indiceAleatorio = distAleatoria(rng);
            return opcoesSemRe[indiceAleatorio].first;
        }
        else {
            // Se a única opção for voltar (beco sem saída), ele obedece
            return opcoesValidas[0].first;
        }
    }
}


// ===================================================================
// MAIN
// ===================================================================
int main() {
    GameState currentState = MENU;
    GameState prevState;
    float volumeGeral = 100;
    GameResources resources;
    int recorde = carregarRecorde();
    bool watermelonRush = false;
    sf::Clock watermelonRushClock;

    // --- Configuração da Janela ---
    sf::RenderWindow jogo(sf::VideoMode(calcularTamanhoInicialJanela()), "Pac-Man");
    jogo.setFramerateLimit(60);
    sf::View gameView(
        sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f),
        sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT)
    );
    aplicarViewProporcional(jogo, gameView);

    // --- Criação do Jogador ---
    Player jogador;
    sf::Clock clock;           // Relógio principal para controlar a velocidade de movimento do jogador
    float delay = delayNormal;   // A velocidade inicial do jogador

    // --- Criação da Lista de Fantasmas ---
    vector<Ghost> ListaFantasmas;

    // --- Inicialização dos Fantasmas ---
    Ghost croc1;
    croc1.x = croc1.xinicial = 11;
    croc1.y = croc1.yinicial = 11;
    croc1.releaseTime = 1; // Sai primeiro

    Ghost croc2;
    croc2.x = croc2.xinicial = 12;
    croc2.y = croc2.yinicial = 11;
    croc2.releaseTime = 3;

    Ghost croc3;
    croc3.x = croc3.xinicial = 13;
    croc3.y = croc3.yinicial = 11;
    croc3.releaseTime = 5;

    Ghost croc4;
    croc4.x = croc4.xinicial = 10;
    croc4.y = croc4.yinicial = 11;
    croc4.releaseTime = 7; // Sai por último

    // Adiciona os fantasmas configurados à lista
    ListaFantasmas.push_back(croc1);
    ListaFantasmas.push_back(croc2);
    ListaFantasmas.push_back(croc3);
    ListaFantasmas.push_back(croc4);

    // --- Setup do Gerador de Números Aleatórios para a IA ---
    random_device rd;
    mt19937 rng(rd()); // Motor de aleatoriedade Mersenne Twister, melhor que o rand() padrão
    uniform_int_distribution<int> dist10(1, 10);

    // --- Carregamento de Todos os Assets (Imagens e Sons) ---
    vector<string> as4direcoes = { "cim", "dir", "bai", "esq" };

    // Carrega as animações para o jogador e para cada fantasma em todas as direções
    for (int i = 0; i < 4; i++) {
        Animacaopacman(i, as4direcoes[i], 4);
        Animacaoswimpacman(i, as4direcoes[i], 4);
        AnimacaoGameOver(i, as4direcoes[i], 10);
        AnimacaoCapEatCroc(i, as4direcoes[i], CAP_EAT_CROC_FRAME_COUNT);
        for (Ghost& croc : ListaFantasmas) {
            AnimacaoFantasma(croc, i, as4direcoes[i], "croc", 4);
            AnimacaoFantasmaSwim(croc, i, as4direcoes[i], "croc", 4);
            AnimacaoFantasmaWeak(croc, i, as4direcoes[i], 4);
            AnimacaoFantasmaWeakSwim(croc, i, as4direcoes[i], 4);
        }
    }

    // Variáveis para gerenciar a animação de Game Over
    Ghost* crocColidido = nullptr; // Ponteiro para saber qual fantasma causou a colisão
    int dirAnimGameOver;
    sf::Vector2f posAnimGameOver;
    int frameAnimGameOver;
    sf::Clock clockAnimGameOver;
    Ghost* crocComido = nullptr;
    int dirAnimEatCroc;
    sf::Vector2f posAnimEatCroc;
    int frameAnimEatCroc = 0;
    sf::Clock clockAnimEatCroc;

    // Carrega a animação da tela de loading
    carregarLoading();
    sf::Sprite loadingSprite(placeholderTexture);
    int currentLoadingFrame = 0;
    sf::Clock transitionClock; // Relógio para controlar a transição de estados

    // Define a textura inicial e a posição visual dos personagens
    jogador.Sprite.setTexture(Texturas[DIR][0], true);
    for (Ghost& croc : ListaFantasmas) {
        croc.sprite.setTexture(croc.animNormal[croc.dir][0], true);
        croc.visualPos = sf::Vector2f(croc.x * SIZE, croc.y * SIZE);
    }

    // Carrega a textura da parede.
    sf::Texture texturaParede;
    if (!texturaParede.loadFromFile("assets/parede.png")) {
        cout << "Erro ao carregar a imagem da parede: parede.png" << endl;
        return -1;
    }
    else {
        cout << "Textura da parede carregada com sucesso!" << endl;
    }
    sf::Sprite spriteParede(texturaParede);

    // Carrega todos os arquivos de áudio em buffers e depois os associa a objetos Sound
    map<string, sf::SoundBuffer>& Buffers = resources.soundBuffers;
    map<string, SoundHandle>& Audios = resources.sounds;
    vector<string> nomesAudios = {
        "point", "kill", "win", "powerup", "swim", "swim-beg", "game-over",
        "life-lost", "button-hover", "button-click", "bg-iniciando"};
    for (auto& nome : nomesAudios)
        carregarAudios(Buffers, nome, ".wav");
    resources.buildSounds();
    Audios["button-hover"].setVolume(50.0f);
    Audios["bg-iniciando"].setVolume(50.0f);

    // --- Carregamento da Música do Menu ---
    sf::Music menuMusic;
    if (!menuMusic.openFromFile("assets/Sounds/bg-music.ogg")) {
        cout << "Erro ao carregar a musica do menu!" << endl;
        return -1;
    }
    else {
        cout << "Musica do menu carregada com sucesso!" << endl;
    }

    menuMusic.setLooping(true); // Faz a música repetir

    // Carrega as texturas do chão (caminho e rio) para o autotiling
    map<string, sf::Texture>& texturasChao = resources.floorTextures;
    sf::Sprite spriteChao(placeholderTexture);
    vector<string> nomesTexturas = {
        "caminho0", "caminho2-canto-dir-bai", "caminho2-canto-dir-cim",
        "caminho2-canto-esq-cim", "caminho2-canto-esq-bai", "caminho2-hor",
        "caminho2-ver", "caminho3-bai", "caminho3-cim", "caminho3-dir",
        "caminho3-esq", "caminho4", "rio2-hor", "rio2-ver", "rio3-bai",
        "rio3-cim", "rio3-dir", "rio3-esq", "rio1-bai", "rio1-cim",
        "rio1-dir", "rio1-esq", "rio0" };
    for (auto& nome : nomesTexturas)
        carregarTexturaChao(texturasChao, nome);

    // Carrega assets (folhas, corações, etc.) 
    map<string, sf::Texture>& Assets = resources.uiTextures;
    sf::Sprite spriteAsset(placeholderTexture);
    vector<string> nomesAssets = {
        "folha", "full-heart", "empty-heart", "blue-heart", "fundo-botao-p",
        "fundo-botao-pressed-p", "fundo-botao-g", "fundo-botao-pressed-g", "melancia"};
    for (auto& nome : nomesAssets)
        carregarAssets(Assets, nome);
    if (!font.openFromFile("fonte.ttf")) {
        cout << "Nao foi possivel carregar o arquivo da fonte!" << endl;
        return -1;
    }
    else {
        cout << "Fonte carregada com sucesso!" << endl;
    }

    // Carrega as texturas da animação de vitória
    carregarAnimacaoVitoria();
    sf::Sprite vitoriaSprite(placeholderTexture);
    int frameVitoria = 0;

    // --- PREPARA ELEMENTOS DO MENU ---
    sf::Font fonteMenu;
    if (!fonteMenu.openFromFile("fonte.ttf")) {
        cout << "Erro ao carregar a fonte do menu!" << endl;
        return -1;
    }
    else {
        cout << "Fonte do menu carregada com sucesso!" << endl;
    }

    sf::Texture menuBackgroundTex;
    if (!menuBackgroundTex.loadFromFile("assets/croc-bg.png")) {
        cout << "Erro ao carregar o fundo do menu!" << endl;
        return -1;
    }
    else {
        cout << "Background carregado com sucesso!" << endl;
    }
    sf::Sprite menuBackground(menuBackgroundTex);

    sf::Vector2u tamanhoTextura = menuBackgroundTex.getSize();
    menuBackground.setScale(sf::Vector2f(
        (float)WINDOW_WIDTH / tamanhoTextura.x,
        (float)WINDOW_HEIGHT / tamanhoTextura.y
    ));

    // ===========================================
    // ============= ELEMENTOS MENU ==============
    // ===========================================
    // Título do Jogo
    sf::Text titulo = criarTextoCentralizado(fonteMenu, "CAPIVARA MAN", 80, sf::Vector2f(WINDOW_WIDTH / 2.0f, 150));
    titulo.setOutlineThickness(4);
    titulo.setStyle(sf::Text::Bold);

    // Botão "Iniciar Jogo"
    sf::Text botaoIniciar = criarTextoCentralizado(fonteMenu, "Iniciar Jogo", 50, sf::Vector2f(WINDOW_WIDTH / 2.0f, 400));

    // Botão "Sair"
    sf::Text botaoSair = criarTextoCentralizado(fonteMenu, "Sair", 50, sf::Vector2f(WINDOW_WIDTH / 2.0f, 600));

    // Botão "Configurações"
    sf::Text botaoConfig = criarTextoCentralizado(fonteMenu, "Configuracoes", 50, sf::Vector2f(WINDOW_WIDTH / 2.0f, 500));

    sf::CircleShape pointer(12, 3); // Raio 12, 3 lados = triângulo para o ponteiro do menu
    pointer.setOutlineColor(sf::Color::Black);
    pointer.setOutlineThickness(2);
    pointer.setRotation(sf::degrees(90)); // Rotaciona para apontar para a direita
    int mousep = 0;
    bool showpointer = false;

    // ===========================================
    // ============ ELEMENTOS PAUSE ==============
    // ===========================================
    
    // Filtro escuro para sobrepor à tela de jogo
    sf::RectangleShape fundoEsc(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    fundoEsc.setFillColor(sf::Color(0, 0, 0, 150));

    // Texto "PAUSADO"
    sf::Text textoPause = criarTextoCentralizado(fonteMenu, "PAUSADO", 90, sf::Vector2f(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f));
    textoPause.setOutlineThickness(4);

    // ===========================================
    // ========= ELEMENTOS FIM DE JOGO ===========
    // ===========================================

    // Usa o mesmo fundo escurecido do pause

    // Título "FIM DE JOGO"
    sf::Text textoFimDeJogo = criarTextoCentralizado(fonteMenu, "FIM DE JOGO", 100, sf::Vector2f(WINDOW_WIDTH / 2.0f, 200));
    textoFimDeJogo.setFillColor(sf::Color::Red);
    textoFimDeJogo.setOutlineThickness(5);

    // Botão "Jogar Novamente"
    sf::Text botaoJogarNovamente = criarTextoCentralizado(fonteMenu, "Jogar Novamente", 50, sf::Vector2f(WINDOW_WIDTH / 2.0f, 400));

    // Botão "Voltar ao Menu"
    sf::Text botaoVoltarMenu = criarTextoCentralizado(fonteMenu, "Voltar ao Menu", 50, sf::Vector2f(WINDOW_WIDTH / 2.0f, 500));

    // ===========================================
    // ========= ELEMENTOS DIFICULDADES ==========
    // ===========================================

    sf::Text tituloDificuldade = criarTextoCentralizado(fonteMenu, "ESCOLHA A DIFICULDADE", 60, sf::Vector2f(WINDOW_WIDTH / 2.0f, 250));
    tituloDificuldade.setOutlineThickness(3);

    std::vector<sf::CircleShape> bolinhasDificuldade(5); // vetor para guardar as 5 bolinhas
    float startX = (WINDOW_WIDTH / 2.0f) - 120; // Posição X inicial para a primeira bolinha
    float posY = 400; // Posição Y de todas as bolinhas
    float spacing = 60; // Espaçamento entre as bolinhas
    int hoveredLevel = 0;
    int prevLevel = 0;

    for (int i = 0; i < 5; ++i) {
        bolinhasDificuldade[i].setRadius(20);
        bolinhasDificuldade[i].setOrigin(sf::Vector2f(20, 20)); // Centraliza a origem
        bolinhasDificuldade[i].setPosition(sf::Vector2f(startX + i * spacing, posY));
        bolinhasDificuldade[i].setFillColor(sf::Color(100, 100, 100)); // Cor "apagada"
        bolinhasDificuldade[i].setOutlineColor(sf::Color::Black);
        bolinhasDificuldade[i].setOutlineThickness(2);
    }

    // Define as cores
    std::vector<sf::Color> coresDificuldade = {
        sf::Color(0x44ce1bFF),
        sf::Color(0xbbdb44FF),
        sf::Color(0xf7e379FF),
        sf::Color(0xf2a134FF),
        sf::Color(0xe51f1fFF)
    };

    // Elementos vitoria
    sf::Text textoVitoria = criarTextoCentralizado(fonteMenu, "VITORIA!", 120, sf::Vector2f(WINDOW_WIDTH / 2.0f, 200));
    textoVitoria.setFillColor(sf::Color::Yellow);
    textoVitoria.setOutlineThickness(5);

    // ===========================================
    // ========= ELEMENTOS CONFIGURACOES ==========
    // ===========================================

    // Texto principal
    sf::Text tituloConfig = criarTextoCentralizado(fonteMenu, "CONFIGURACOES", 80, sf::Vector2f(WINDOW_WIDTH / 2.0f, 180));
    tituloConfig.setOutlineThickness(5);

    // Texto volume
    sf::Text labelVolume = criarTextoCentralizado(fonteMenu, "Volume:", 50, sf::Vector2f(WINDOW_WIDTH / 2.0f, 380));

    // Botão voltar
    sf::Text botaoVoltar = criarTextoCentralizado(fonteMenu, "Voltar", 50, sf::Vector2f(WINDOW_WIDTH / 2.0f, 600));

    // Controle deslizante
    sf::RectangleShape volumeTrack(sf::Vector2f(400, 10)); // 300px de largura, 10px de altura
    volumeTrack.setFillColor(sf::Color(80, 80, 80));
    volumeTrack.setOutlineColor(sf::Color::Black);
    volumeTrack.setOutlineThickness(2);
    sf::FloatRect textRect = volumeTrack.getLocalBounds();
    volumeTrack.setOrigin(sf::Vector2f(textRect.position.x + textRect.size.x / 2.0f, textRect.position.y + textRect.size.y / 2.0f));
    volumeTrack.setPosition(sf::Vector2f(labelVolume.getPosition().x, labelVolume.getPosition().y + 50));

    // Bolinha deslizante
    sf::CircleShape volumeHandle(15); // Raio de 15px
    volumeHandle.setFillColor(sf::Color::White);
    volumeHandle.setOutlineColor(sf::Color::Black);
    volumeHandle.setOutlineThickness(2);
    volumeHandle.setOrigin(sf::Vector2f(15, 15)); // Centraliza a origem para posicionamento fácil
    volumeHandle.setPosition(sf::Vector2f(volumeTrack.getPosition().x + volumeTrack.getSize().x / 2.f, volumeTrack.getPosition().y));

    // Texto que mostra o valor (ex: 50%)
    sf::Text valorVolumeText(fonteMenu, "50", 40);
    valorVolumeText.setFillColor(sf::Color::White);
    valorVolumeText.setOutlineColor(sf::Color::Black);
    valorVolumeText.setOutlineThickness(2);
    valorVolumeText.setPosition(sf::Vector2f(volumeTrack.getPosition().x + volumeTrack.getSize().x / 2.f + 30, volumeTrack.getPosition().y - 15));

    sf::Text recordeText(font, "", 24);
    recordeText.setFillColor(sf::Color::Black);
    recordeText.setPosition(sf::Vector2f(20, MAP_HEIGHT + 12));

    sf::Text estadoPowerupText(font, "", 24);
    estadoPowerupText.setFillColor(sf::Color(0x0d5c63FF));
    estadoPowerupText.setPosition(sf::Vector2f(MAP_WIDTH / 2.f - 140, MAP_HEIGHT + 12));

    sf::Text placarFinalText = criarTextoCentralizado(fonteMenu, "", 38, sf::Vector2f(WINDOW_WIDTH / 2.0f, 305));
    placarFinalText.setOutlineThickness(2);

    // Controle do arraste
    bool isDraggingVolume = false;

    // ===========================================
    // ========= LOOP PRINCIPAL DO JOGO ==========
    // ===========================================
    while (jogo.isOpen()) {
        // Calcula o delta time, crucial para que a velocidade do jogo seja independente do hardware
        float deltaTime = deltaClock.restart().asSeconds();

        // --- Processamento de Eventos (Input do Usuário) ---
        while (const std::optional<sf::Event> event = jogo.pollEvent()) {
            const auto* keyPressed = event->getIf<sf::Event::KeyPressed>();
            const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>();
            const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>();
            const auto* resized = event->getIf<sf::Event::Resized>();
            // Evento para fechar a janela (no 'X')
            if (event->is<sf::Event::Closed>())
                jogo.close();
            if (resized)
                aplicarViewProporcional(jogo, gameView);

            if (keyPressed) {
                if (keyPressed->code == sf::Keyboard::Key::P) {
                    if (currentState == JOGANDO) {
                        // Se está jogando, pausa o jogo
                        currentState = PAUSADO;
                        cout << "Jogo Pausado" << endl;
                        Audios["button-click"].play();
                        menuMusic.play();
                    }
                    else if (currentState == PAUSADO) {
                        // Se está pausado, volta a jogar
                        currentState = JOGANDO;
                        cout << "Jogo Retomado" << endl;
                        deltaClock.restart(); // Reinicia o deltaClock para evitar um "salto" no tempo
                        Audios["button-click"].play();
                        menuMusic.stop();
                    }
                }
            }
            if (currentState == CONFIGURACOES) {
                // Quando o botão do mouse é pressionado
                if (mouseButtonPressed) {
                    if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                        sf::Vector2f clickPos = jogo.mapPixelToCoords(sf::Mouse::getPosition(jogo));
                        // Verifica se o clique foi na barra ou na bolinha
                        if (volumeTrack.getGlobalBounds().contains(clickPos) || volumeHandle.getGlobalBounds().contains(clickPos)) {
                            isDraggingVolume = true;
                        }
                    }
                }
                // Quando o botão do mouse é solto
                if (mouseButtonReleased) {
                    if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
                        isDraggingVolume = false; // Para de arrastar
                    }
                }
            }

            if (currentState == PAUSADO) {
                if (mouseButtonPressed) {
                    if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                        sf::Vector2i pixelPos = sf::Mouse::getPosition(jogo);
                        sf::Vector2f clickPos = jogo.mapPixelToCoords(pixelPos);

                        // Se foi no botão "Configurações"
                        if (botaoConfig.getGlobalBounds().contains(clickPos)) {

                            prevState = currentState; // Guarda onde estava
                            currentState = CONFIGURACOES;
                            Audios["button-click"].play();
                            mousep = 0;
                        }
                    }
                }
            }

            // Lógica de input para o estado MENU.
            if (currentState == MENU) {
                if (mousep != 0) {
                    showpointer = true;
                }
                else {
                    showpointer = false;
                }
                if (mouseButtonPressed) {
                    if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                        sf::Vector2i pixelPos = sf::Mouse::getPosition(jogo);
                        sf::Vector2f clickPos = jogo.mapPixelToCoords(pixelPos);

                        // Se o clique foi no botão "Iniciar Jogo"
                        if (botaoIniciar.getGlobalBounds().contains(clickPos)) {
                            currentState = ESCOLHA_DIFICULDADE;
                            Audios["button-click"].play();
                            transitionClock.restart();
                        }

                        // Se o clique foi no botão "Sair"
                        if (botaoSair.getGlobalBounds().contains(clickPos)) {
                            Audios["button-click"].play();
                            jogo.close();
                        }

                        // Se foi no botão "Configurações"
                        if (botaoConfig.getGlobalBounds().contains(clickPos)) {
                            cout << "Configuracoes" << endl;
                            prevState = currentState; // Guarda onde estava
                            currentState = CONFIGURACOES;
                            Audios["button-click"].play();
                            mousep = 0;
                        }
                    }
                }
                if (keyPressed) {
                    if (keyPressed->code == sf::Keyboard::Key::Enter && mousep != 2) {
                        currentState = ESCOLHA_DIFICULDADE;
                        Audios["button-click"].play();
                        transitionClock.restart();
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::Enter) {
                        Audios["button-click"].play();
                        jogo.close();
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::Down || keyPressed->code == sf::Keyboard::Key::Up) {
                        // Lógica de hover para o botão INICIAR
                        if (mousep != 1) {
                            mousep = 1;
                            Audios["button-hover"].play();

                            botaoIniciar.setFillColor(sf::Color(0xf2b24bFF)); // Cor de destaque
                            pointer.setFillColor(sf::Color(0xf2b24bFF));
                            // Posiciona o ponteiro à esquerda do botão
                posicionarPonteiro(pointer, botaoIniciar);
                        }
                        else {
                            mousep = 2;
                            Audios["button-hover"].play();

                            botaoSair.setFillColor(sf::Color(0xf2b24bFF));
                            pointer.setFillColor(sf::Color(0xf2b24bFF));
                posicionarPonteiro(pointer, botaoSair);
                        }
                    }
                }
            }

            if (currentState == ESCOLHA_DIFICULDADE) {
                if (mouseButtonPressed) {
                    if (mouseButtonPressed->button == sf::Mouse::Button::Left && hoveredLevel != 0) {
                        sf::Vector2f clickPos = jogo.mapPixelToCoords(sf::Mouse::getPosition(jogo));

                        if (hoveredLevel > 0) {
                            dificuldadeJogo = hoveredLevel;
                            cout << "Dificuldade selecionada: " << hoveredLevel << endl;

                            pontuacao = 0;
                            watermelonRush = false;
                            currentState = INICIANDO;
                            Audios["button-click"].play();
                            transitionClock.restart();
                            menuMusic.stop();
                        }
                        
                    }
                }
                if (keyPressed) {
                    if (keyPressed->code == sf::Keyboard::Key::Right && hoveredLevel < 5) {
                        hoveredLevel++;
                        Audios["button-hover"].play();
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::Left && hoveredLevel > 1) {
                        hoveredLevel--;
                        Audios["button-hover"].play();
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::Enter && hoveredLevel != 0) {
                        dificuldadeJogo = hoveredLevel;
                        cout << "Dificuldade selecionada: " << hoveredLevel << endl;

                        pontuacao = 0;
                        watermelonRush = false;
                        currentState = INICIANDO;
                        Audios["button-click"].play();
                        transitionClock.restart();
                        menuMusic.stop();
                    }
                }
            }

            if (currentState == TELA_FIM_DE_JOGO || currentState == VITORIA) {
                if (mousep != 0) {
                    showpointer = true;
                }
                else {
                    showpointer = false;
                }
                if (mouseButtonPressed) {
                    if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                        sf::Vector2i pixelPos = sf::Mouse::getPosition(jogo);
                        sf::Vector2f clickPos = jogo.mapPixelToCoords(pixelPos);

                        // Se o clique foi no botão "Jogar Novamente"
                        if (botaoJogarNovamente.getGlobalBounds().contains(clickPos)) {
                            currentState = ESCOLHA_DIFICULDADE;
                            Audios["button-click"].play();
                            transitionClock.restart();
                        }

                        // Se o clique foi no botão "Voltar Menu"
                        if (botaoVoltarMenu.getGlobalBounds().contains(clickPos)) {
                            currentState = MENU;
                            Audios["button-click"].play();
                            mousep = 0;
                            break;
                        }
                    }
                }
                if (keyPressed) {
                    if (keyPressed->code == sf::Keyboard::Key::Enter && mousep != 2) {
                        currentState = ESCOLHA_DIFICULDADE;
                        Audios["button-click"].play();
                        transitionClock.restart();
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::Enter) {
                        if (mousep == 2) {
                            currentState = MENU;
                            Audios["button-click"].play();
                            mousep = 0;
                        }
                        else if (mousep == 1) {
                            currentState = ESCOLHA_DIFICULDADE;
                            Audios["button-click"].play();
                            transitionClock.restart(); 
                        }
                        mousep = 0;
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::Down || keyPressed->code == sf::Keyboard::Key::Up) {
                        // Lógica de hover para o botão "Jogar Novamente"
                        if (mousep != 1) {
                            mousep = 1;
                            Audios["button-hover"].play();

                            botaoJogarNovamente.setFillColor(sf::Color(0xf2b24bFF)); // Cor de destaque
                            pointer.setFillColor(sf::Color(0xf2b24bFF));
                            // Posiciona o ponteiro à esquerda do botão
                posicionarPonteiro(pointer, botaoJogarNovamente);
                        }
                        else {
                            mousep = 2;
                            Audios["button-hover"].play();

                            botaoVoltarMenu.setFillColor(sf::Color(0xf2b24bFF));
                            pointer.setFillColor(sf::Color(0xf2b24bFF));
                posicionarPonteiro(pointer, botaoVoltarMenu);
                        }
                    }
                }
            }

            // Lógica de input para o estado JOGANDO
            if (currentState == JOGANDO) {
                if (keyPressed) {
                    if (keyPressed->code == sf::Keyboard::Key::Left) jogador.wait = ESQ;
                    else if (keyPressed->code == sf::Keyboard::Key::Right) jogador.wait = DIR;
                    else if (keyPressed->code == sf::Keyboard::Key::Up) jogador.wait = CIM;
                    else if (keyPressed->code == sf::Keyboard::Key::Down) jogador.wait = BAI;
                    else if (keyPressed->code == sf::Keyboard::Key::R) {
                        // Tecla de atalho para resetar o jogo
                        Reset(jogador, ListaFantasmas, 1);
                        Audios["button-click"].play();
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::T) {
                        for (auto& croc : ListaFantasmas) {
                            croc.dificuldade = 0;
                            cout << "Mudando para modo de teste. Dificuldade: 0" << endl;
                        }
                        Audios["button-click"].play();
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::M) {
                        currentState = MENU;
                        Audios["button-click"].play();
                    }
                }
            }
        }

        // --- Máquina de Estados Principal ---
        switch (currentState) {
        // TELA MENU
        case MENU: {
            if (menuMusic.getStatus() != sf::Music::Status::Playing) {
                menuMusic.play();
            }

            sf::Vector2i pixelPos = sf::Mouse::getPosition(jogo);
            sf::Vector2f mousePos = jogo.mapPixelToCoords(pixelPos);

            // Lógica de hover para o botão INICIAR
            if (botaoIniciar.getGlobalBounds().contains(mousePos)) {
                if (mousep != 1) { // Evita tocar o som repetidamente  
                    mousep = 1;
                    Audios["button-hover"].play();
                }
                botaoIniciar.setFillColor(sf::Color(0xf2b24bFF)); // Cor de destaque
                pointer.setFillColor(sf::Color(0xf2b24bFF));
                // Posiciona o ponteiro à esquerda do botão
                posicionarPonteiro(pointer, botaoIniciar);
                showpointer = true;
            }
            else if (mousep != 1) {
                botaoIniciar.setFillColor(sf::Color::White); // Cor padrão
            }

            // Lógica de hover para o botão SAIR
            if (botaoSair.getGlobalBounds().contains(mousePos)) {
                if (mousep != 2) {
                    mousep = 2;
                    Audios["button-hover"].play();
                }
                botaoSair.setFillColor(sf::Color(0xf2b24bFF));
                pointer.setFillColor(sf::Color(0xf2b24bFF));
                posicionarPonteiro(pointer, botaoSair);
                showpointer = true;
            }
            else if (mousep != 2) {
                botaoSair.setFillColor(sf::Color::White);
            }

            if (botaoConfig.getGlobalBounds().contains(mousePos)) {
                if (mousep != 3) {
                    mousep = 3;
                    Audios["button-hover"].play();
                }
                botaoConfig.setFillColor(sf::Color(0xf2b24bFF));
                pointer.setFillColor(sf::Color(0xf2b24bFF));
                posicionarPonteiro(pointer, botaoConfig);
                showpointer = true;
            }
            else if (mousep != 3) {
                botaoConfig.setFillColor(sf::Color::White);
            }

            break;
        }
        // TELA DE DIFICULDADES
        case ESCOLHA_DIFICULDADE: {
            sf::Vector2i pixelPos = sf::Mouse::getPosition(jogo);
            sf::Vector2f worldPos = jogo.mapPixelToCoords(pixelPos);

            // Verifica o hover da direita para a esquerda para que o nível mais alto tenha prioridade
            for (int i = 4; i >= 0; --i) {
                if (bolinhasDificuldade[i].getGlobalBounds().contains(worldPos)) {
                    hoveredLevel = i + 1; // Nível é 1-5, índice é 0-4

                    if (prevLevel != hoveredLevel) {
                        Audios["button-hover"].play();
                    }

                    prevLevel = hoveredLevel;
                    break; // Encontrou o mais alto, não precisa checar os outros
                }
            }

            // Pinta as bolinhas com base no nível que o mouse está sobre
            for (int i = 0; i < 5; ++i) {
                if (i < hoveredLevel) {
                    bolinhasDificuldade[i].setFillColor(coresDificuldade.at(hoveredLevel - 1)); // Cor "acesa"
                }
                else {
                    bolinhasDificuldade[i].setFillColor(sf::Color(100, 100, 100)); // Cor "apagada"
                }
            }

            if (Audios["win"].getStatus() == sf::Sound::Status::Playing) {
                Audios["win"].stop();
            }

            if (menuMusic.getStatus() != sf::Music::Status::Playing) {
                menuMusic.play();
            }
            break;
        }
        // TELA DE LOADING
        case INICIANDO: {
            if (Audios["bg-iniciando"].getStatus() != sf::Sound::Status::Playing)
                Audios["bg-iniciando"].play();
            float tempoDecorrido = transitionClock.getElapsedTime().asSeconds();
            currentLoadingFrame = static_cast<int>(tempoDecorrido / FRAME_DELAY);

            // Quando a animação de loading terminar
            if (tempoDecorrido >= LOADING_DURATION) {
                Reset(jogador, ListaFantasmas, 1); // Reseta tudo para uma nova partida
                deltaClock.restart();
                watermelonRush = false;
                for (auto& croc : ListaFantasmas) {
                    croc.isWeak = false;
                    croc.releaseClock.restart(); // Zera os timers de liberação dos fantasmas
                }
                // Sincroniza a posição visual com a lógica.
                jogador.visualPos.x = jogador.posx * SIZE;
                jogador.visualPos.y = jogador.posy * SIZE;
                jogador.dead = false;

                Audios["bg-iniciando"].stop();
                currentState = JOGANDO; // começa o jogo
            }

            break;
        }
        // Lógica Principal do Jogo
        case JOGANDO: {
            // --- Atualização do Estado do Jogo ---
            if (watermelonRush && watermelonRushClock.getElapsedTime().asSeconds() >= WATERMELON_RUSH_DURATION) {
                watermelonRush = false;
                for (auto& croc : ListaFantasmas) {
                    croc.isWeak = false;
                    croc.sprite.setColor(sf::Color::White);
                }
                cout << "Powerup terminou." << endl;
            }

            // Ajusta a velocidade do jogador com base no terreno
            if (mapa[jogador.posy][jogador.posx] == '2') { // Se está na água
                delay = delayLento;
                delayAnim = SIZE / delay;
            }
            else { // Se está em terra
                delay = delayNormal;
                delayAnim = SIZE / delay;
            }

            // Movimentação do Jogador, controlada por tempo para ser consistente
            if (clock.getElapsedTime().asSeconds() > delay && !stop) {
                clock.restart();

                // Processa o 'wait'. Se o movimento desejado é válido, muda a direção
                if (jogador.wait == DIR && jogador.posx < 24 && mapa[jogador.posy][jogador.posx + 1] != '1') {
                    jogador.dir = true;
                    jogador.esq = jogador.cima = jogador.baixo = false;
                    jogador.wait = -1;
                }
                else if (jogador.wait == ESQ && jogador.posx > 0 && mapa[jogador.posy][jogador.posx - 1] != '1') {
                    jogador.esq = true;
                    jogador.dir = jogador.cima = jogador.baixo = false;
                    jogador.wait = -1;
                }
                else if (jogador.wait == CIM && jogador.posy > 0 && mapa[jogador.posy - 1][jogador.posx] != '1') {
                    jogador.cima = true;
                    jogador.esq = jogador.dir = jogador.baixo = false;
                    jogador.wait = -1;
                }
                else if (jogador.wait == BAI && jogador.posy < 12 && mapa[jogador.posy + 1][jogador.posx] != '1') {
                    jogador.baixo = true;
                    jogador.esq = jogador.dir = jogador.cima = false;
                    jogador.wait = -1;
                }

                // Calcula a próxima posição lógica com base na direção atual
                int nx = jogador.posx + jogador.dir - jogador.esq;
                int ny = jogador.posy + jogador.baixo - jogador.cima;

                // Lógica de Teleporte
                if (ny < 0) {
                    ny = ALT - 1;
                    jogador.visualPos.y = ALT * SIZE; // Ajusta a posição visual para a interpolação funcionar
                }
                else if (ny > ALT - 1) {
                    ny = 0;
                    jogador.visualPos.y = -SIZE;
                }
                if (nx < 0) {
                    nx = LAR - 1;
                    jogador.visualPos.x = LAR * SIZE;
                }
                else if (nx > LAR - 1) {
                    nx = 0;
                    jogador.visualPos.x = -SIZE;
                }

                // Guarda a posição antiga para a detecção de colisão cruzada
                jogador.prev_posx = jogador.posx;
                jogador.prev_posy = jogador.posy;

                // Se o movimento for para uma célula válida (não parede), atualiza a posição lógica
                if (nx >= 0 && nx < LAR && ny >= 0 && ny < ALT && mapa[ny][nx] != '1') {
                    jogador.posx = nx;
                    jogador.posy = ny;
                }
                else {
                    // Se bateu na parede, para o movimento
                    jogador.dir = jogador.esq = jogador.cima = jogador.baixo = false;
                }

                // Lógica de Coleta de Pontos
                if (mapaFolhas[jogador.posy][jogador.posx]) {
                    mapaFolhas[jogador.posy][jogador.posx] = false;  // Remove o asset do mapa
                    pontuacao += ppf;                                // Adiciona pontos
                    numPontos--;                                     // Tira uma ponto do total
                    if (mapa[jogador.posy][jogador.posx] != '4')
                        Audios["point"].play();                      // Toca o som de coleta
                    else {
                        Audios["powerup"].play();
                        watermelonRush = true;
                        watermelonRushClock.restart();
                        for (auto& croc : ListaFantasmas) {
                            croc.isWeak = true;
                        }
                        if (!melan[0]) {
                            melan[0] = 1;
                        }
                        else if (!melan[1]) {
                            melan[1] = 1;
                        }
                        cout << "Powerup coletado. 1 vida extra adicionada!" << endl;
                    }
                    cout << "Point buffer tocando. Pontos restantes: " << numPontos << endl;

                    if (numPontos == 0) {
                        cout << "VITORIA! Todos os pontos foram coletados." << endl;
                        atualizarRecorde(pontuacao, recorde);
                        Audios["win"].play(); // Toca o som de vitória
                        currentState = VITORIA;
                        mousep = 0;
                        showpointer = false;
                        clockVitoria.restart(); // Inicia o relógio da animação de vitoria
                    }
                }

                // Atualiza o texto da pontuação
                scoreText.setFont(font);
                scoreText.setString("Pontos:" + to_string(pontuacao));
                scoreText.setCharacterSize(35);
                scoreText.setFillColor(sf::Color::Black);

                // Atualiza a última direção válida do jogador para a animação
                if (jogador.dir) jogador.ultimadir = DIR;
                else if (jogador.esq) jogador.ultimadir = ESQ;
                else if (jogador.cima) jogador.ultimadir = CIM;
                else if (jogador.baixo) jogador.ultimadir = BAI;
            }

            // --- Movimentação dos Fantasmas ---

            // A IA precisa do mapa de distâncias atualizado a cada frame
            calcularDistancias(jogador, mapa, dist);

            for (Ghost& croc : ListaFantasmas) {
                // Só move o fantasma se ele já foi liberado e o jogo não está pausado
                if (croc.release && !stop) {
                    // Ajusta a velocidade do fantasma com base no terreno
                    if (mapa[croc.y][croc.x] == '2') { // Na água, o fantasma é mais rápido
                        croc.delay = croc.delayAgua;
                        croc.delayAnim = SIZE / croc.delay;
                    }
                    else {
                        croc.delay = croc.delayNormal;
                        croc.delayAnim = SIZE / croc.delay;
                    }
                    if (watermelonRush) {
                        croc.delay *= 1.8f;
                        croc.delayAnim = SIZE / croc.delay;
                    }

                    // Movimentação do Fantasma, também controlada por tempo
                    if (croc.Clock.getElapsedTime().asSeconds() > croc.delay) {
                        croc.Clock.restart();

                        // A IA  decide o próximo movimento
                        croc.dir = melhorDirecao(croc, dist, mapa, rng);

                        // Calcula a próxima posição
                        int nx = croc.x + dx[croc.dir];
                        int ny = croc.y + dy[croc.dir];

                        // --- Detecção de Colisão ---
                        // Condição 1: O fantasma e o jogador se moveram para a mesma célula
                        bool mesma_casa = (nx == jogador.posx && ny == jogador.posy);
                        // Condição 2: Eles se cruzaram, trocando de lugar no mesmo movimento
                        bool trocaram_de_lugar = ((croc.x == jogador.prev_posx && croc.y == jogador.prev_posy) || (croc.x == jogador.posx && croc.y == jogador.posy));

                        if (mesma_casa || trocaram_de_lugar)
                        {
                            if (watermelonRush) {
                                pontuacao += CROCODILE_BONUS;
                                atualizarRecorde(pontuacao, recorde);
                                Audios["kill"].play();
                                currentState = COMENDO_CROC;
                                crocComido = &croc;
                                dirAnimEatCroc = jogador.ultimadir;
                                posAnimEatCroc = jogador.visualPos - offsetCapivaraNaAnimacaoCroc(dirAnimEatCroc);
                                frameAnimEatCroc = 0;
                                clockAnimEatCroc.restart();
                                cout << "Crocodilo mandado para a base. Bonus: " << CROCODILE_BONUS << endl;
                                break;
                            }

                            // COLISÃO
                            currentState = GAME_OVER; // Muda o estado do jogo
                            watermelonRush = false;
                            for (auto& outroCroc : ListaFantasmas) {
                                outroCroc.isWeak = false;
                                outroCroc.sprite.setColor(sf::Color::White);
                            }
                            Audios["life-lost"].play();

                            // Prepara as variáveis para a animação de "comer" o jogador
                            crocColidido = &croc;
                            dirAnimGameOver = croc.dir;
                            // A posição da animação precisa de ajuste para ficar visualmente correta
                            if (dirAnimGameOver == 0) {
                                posAnimGameOver.x = croc.visualPos.x;
                                posAnimGameOver.y = jogador.visualPos.y;
                            }
                            else if (dirAnimGameOver == 3) {
                                posAnimGameOver.x = jogador.visualPos.x;
                                posAnimGameOver.y = croc.visualPos.y;
                            }
                            else
                                posAnimGameOver = croc.visualPos;
                            frameAnimGameOver = 0;
                            clockAnimGameOver.restart();

                            break; // Sai do loop 'for' para não processar outros fantasmas neste frame
                        }

                        // Se não houve colisão, atualiza a posição do fantasma
                        croc.x = nx;
                        croc.y = ny;

                        // Teleporte
                        if (croc.y < 0) {
                            croc.y = ALT - 1;
                            croc.visualPos.y = ALT * SIZE;
                        }
                        else if (croc.y > ALT - 1) {
                            croc.y = 0;
                            croc.visualPos.y = -SIZE;
                        }
                        if (croc.x < 0) {
                            croc.x = LAR - 1;
                            croc.visualPos.x = LAR * SIZE;
                        }
                        else if (croc.x > LAR - 1) {
                            croc.x = 0;
                            croc.visualPos.x = -SIZE;
                        }
                    }
                }
                // Se o fantasma ainda não foi liberado, verifica se o tempo de espera já passou
                else if (croc.releaseClock.getElapsedTime().asSeconds() >= croc.releaseTime) {
                    croc.release = true; // Libera o fantasma
                }
            }

            break;
        }
        // PAUSADO, não faz nada
        case PAUSADO: {
            sf::Vector2i pixelPos = sf::Mouse::getPosition(jogo);
            sf::Vector2f mousePos = jogo.mapPixelToCoords(pixelPos);

            if (botaoConfig.getGlobalBounds().contains(mousePos)) {
                if (mousep != 3) {
                    mousep = 3;
                    Audios["button-hover"].play();
                }
                botaoConfig.setFillColor(sf::Color(0xf2b24bFF));
                pointer.setFillColor(sf::Color(0xf2b24bFF));
                posicionarPonteiro(pointer, botaoConfig);
                showpointer = true;
            }
            else {
                botaoConfig.setFillColor(sf::Color::White);
                mousep = 0;
                showpointer = false;
            }

            break;
        }
        case CONFIGURACOES: {
            sf::Vector2i pixelPos = sf::Mouse::getPosition(jogo);
            sf::Vector2f mousePos = jogo.mapPixelToCoords(pixelPos);

            if (isDraggingVolume) {
                showpointer = false;

                // Limita a posição da bolinha aos limites da trilha
                float trackX_start = volumeTrack.getPosition().x - volumeTrack.getSize().x / 2.f;
                float trackX_end = trackX_start + volumeTrack.getSize().x;
                float newHandleX = std::max(trackX_start, std::min(mousePos.x, trackX_end));

                // Atualiza a posição visual da bolinha
                volumeHandle.setPosition(sf::Vector2f(newHandleX, volumeTrack.getPosition().y));

                // Converte a posição da bolinha (em pixels) para um valor de volume (0 a 100)
                float handleRelativePos = volumeHandle.getPosition().x - trackX_start;
                float progress = handleRelativePos / volumeTrack.getSize().x;
                volumeGeral = progress * 100.f;

                // Aplica o novo valor
                setVolumeGeral(volumeGeral, menuMusic, Audios);
            }

            // Atualiza o texto do valor do volume
            valorVolumeText.setString(to_string((int)volumeGeral) + "%");

            sf::Vector2f worldPos = jogo.mapPixelToCoords(sf::Mouse::getPosition(jogo));

            // Lógica de hover para o botão Voltar
            if (botaoVoltar.getGlobalBounds().contains(mousePos)) {
                if (mousep != 1) { // Evita tocar o som repetidamente
                    mousep = 1;
                    Audios["button-hover"].play();
                }
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    currentState = prevState; // Volta para a tela anterior
                    Audios["button-click"].play();
                    mousep = 3;
                }
                botaoVoltar.setFillColor(sf::Color(0xf2b24bFF)); // Cor de destaque
                pointer.setFillColor(sf::Color(0xf2b24bFF));
                // Posiciona o ponteiro à esquerda do botão
                posicionarPonteiro(pointer, botaoVoltar);
                showpointer = true;
            }
            else {
                botaoVoltar.setFillColor(sf::Color::White); // Cor padrão
                mousep = 0;
                showpointer = false;
            }
            break;
        }
        // TELA PARA GAME_OVER (animação de morte)
        case GAME_OVER: {
            // Avança o frame da animação de morte em intervalos regulares
            if (clockAnimGameOver.getElapsedTime().asSeconds() > ANIMATION_SPEED) {
                frameAnimGameOver++;
                clockAnimGameOver.restart();
            }

            // Quando a animação de morte terminar
            if (frameAnimGameOver >= 10) {
                Reset(jogador, ListaFantasmas, 0);
                if (!jogador.dead) { // Se ainda tem vidas
                    currentState = JOGANDO; // Volta para o jogo
                }
                else { // Se não tem mais vidas
                    cout << "Fim de Jogo!" << endl;
                    atualizarRecorde(pontuacao, recorde);
                    currentState = TELA_FIM_DE_JOGO;
                    Audios["game-over"].play();
                    mousep = 0;
                }
            }
            break;
        }
        case COMENDO_CROC: {
            if (clockAnimEatCroc.getElapsedTime().asSeconds() > CAP_EAT_CROC_FRAME_DELAY) {
                frameAnimEatCroc++;
                clockAnimEatCroc.restart();
            }

            if (frameAnimEatCroc >= CAP_EAT_CROC_FRAME_COUNT) {
                if (crocComido) {
                    mandarFantasmaParaBase(*crocComido);
                    crocComido->isWeak = watermelonRush;
                    crocComido = nullptr;
                }
                currentState = JOGANDO;
            }
            break;
        }
        // TELA DE VITORIA
        case VITORIA: {
            // Lógica da animação de confete (igual à do loading)
            const float a_frame_delay = 0.05f; // Delay entre frames do confete
            frameVitoria = static_cast<int>(clockVitoria.getElapsedTime().asSeconds() / a_frame_delay);

            // Faz a animação repetir
            if (frameVitoria >= 191) {
                frameVitoria = 0;
                clockVitoria.restart();
            }
        }
        // TELA DE GAME OVER (definitivo)
        case TELA_FIM_DE_JOGO: {
            sf::Vector2i pixelPos = sf::Mouse::getPosition(jogo);
            sf::Vector2f mousePos = jogo.mapPixelToCoords(pixelPos);

            // Lógica de hover para o botão Jogar Novamente
            if (botaoJogarNovamente.getGlobalBounds().contains(mousePos)) {
                if (mousep != 1) { // Evita tocar o som repetidamente
                    mousep = 1;
                    Audios["button-hover"].play();
                }
                botaoJogarNovamente.setFillColor(sf::Color(0xf2b24bFF)); // Cor de destaque
                pointer.setFillColor(sf::Color(0xf2b24bFF));
                // Posiciona o ponteiro à esquerda do botão
                posicionarPonteiro(pointer, botaoJogarNovamente);
                showpointer = true;
            }
            else if (mousep != 1) {
                botaoJogarNovamente.setFillColor(sf::Color::White); // Cor padrão
            }

            // Lógica de hover para o botão Voltar Menu
            if (botaoVoltarMenu.getGlobalBounds().contains(mousePos)) {
                if (mousep != 2) {
                    mousep = 2;
                    Audios["button-hover"].play();
                }
                botaoVoltarMenu.setFillColor(sf::Color(0xf2b24bFF));
                pointer.setFillColor(sf::Color(0xf2b24bFF));
                posicionarPonteiro(pointer, botaoVoltarMenu);
                showpointer = true;
            }
            else if (mousep != 2) {
                botaoVoltarMenu.setFillColor(sf::Color::White);
            }
            break;
        }
        }
        
        // --- Lógica de Animação ---
        // Move os sprites suavemente de uma célula para a outra

        // 1. Calcula o vetor de movimento do jogador (da posição visual atual para a lógica)
        sf::Vector2f alvoJogador(jogador.posx * SIZE, jogador.posy * SIZE);
        sf::Vector2f dirmovJogador = alvoJogador - jogador.visualPos;
        float distanciaJogador = abs(dirmovJogador.x) + abs(dirmovJogador.y);

        // 2. Move o sprite do jogador um pouco em direção ao alvo
        if (distanciaJogador > 0.1f) { // Se não estiver perto o suficiente do alvo
            float distmov = delayAnim * deltaTime; // Distância a mover neste frame
            jogador.visualPos += dirmovJogador / distanciaJogador * min(distmov, distanciaJogador);
        }
        else {
            jogador.visualPos = alvoJogador; // Trava na posição exata para evitar imprecisão
        }

        // 3. Move cada fantasma da mesma forma
        bool todosFantasmasResetados = true;
        for (Ghost& croc : ListaFantasmas) {
            sf::Vector2f alvoCroc(croc.x * SIZE, croc.y * SIZE);
            sf::Vector2f dirMovCroc = alvoCroc - croc.visualPos;
            float distCroc = abs(dirMovCroc.x) + abs(dirMovCroc.y);

            if (distCroc > 0.1f) {
                float distmov = croc.delayAnim * deltaTime;
                croc.visualPos += dirMovCroc / distCroc * min(distmov, distCroc);
                todosFantasmasResetados = false; // Se ao menos um fantasma ainda está se movendo, a transição não acabou
            }
            else {
                croc.visualPos = alvoCroc; // Trava na posição exata
            }
        }

        // 4. Se o jogo estava pausado (stop) e todos chegaram ao destino
        if (stop && todosFantasmasResetados && distanciaJogador < 0.1f) {
            cout << "Transicao do reset finalizada! Iniciando timers." << endl;
            for (auto& croc : ListaFantasmas) {
                croc.releaseClock.restart(); // Reinicia os timers de liberação
                croc.release = false;
            }
            stop = false; // ...libera o jogo para continuar
        }

        // --- Lógica de troca de frames dos Sprites ---
        bool isMoving = jogador.dir || jogador.esq || jogador.cima || jogador.baixo;
        // Verifica se o centro do sprite do jogador está sobre uma célula de água
        bool pacNaAgua = estaNaAgua(jogador.visualPos);

        if ((isMoving || pacNaAgua) && !stop && currentState != COMENDO_CROC) {
            if (animationClock.getElapsedTime().asSeconds() > ANIMATION_SPEED) {
                // Escolhe o conjunto de texturas (terra ou água) e avança o frame
                const auto& texPac = pacNaAgua ? Texturasswim[jogador.ultimadir] : Texturas[jogador.ultimadir];
                jogador.animationFrame = (jogador.animationFrame + 1) % texPac.size();
            }
            // Toca ou para o som de nado.
            if (pacNaAgua && Audios["swim"].getStatus() != sf::Sound::Status::Playing && !jogador.dead && currentState == JOGANDO) {
                Audios["swim"].play();
            }
            else if (!pacNaAgua && Audios["swim"].getStatus() == sf::Sound::Status::Playing || currentState != JOGANDO) {
                Audios["swim"].stop();
            }
        }
        else if (!pacNaAgua) {
            jogador.animationFrame = 3;
        }

        // Animação dos sprites dos Fantasmas (ocorre para todos simultaneamente)
        if (animationClock.getElapsedTime().asSeconds() > ANIMATION_SPEED) {
            for (Ghost& croc : ListaFantasmas) {
                bool crocNaAgua = estaNaAgua(croc.visualPos);
                const auto& texCroc = croc.isWeak
                    ? (crocNaAgua ? croc.animWeakSwim[croc.dir] : croc.animWeak[croc.dir])
                    : (crocNaAgua ? croc.animSwim[croc.dir] : croc.animNormal[croc.dir]);
                croc.animFrame = (croc.animFrame + 1) % texCroc.size();
            }
            animationClock.restart(); // Reinicia o relógio global de animação
        }

        atualizarRecorde(pontuacao, recorde);
        scoreText.setFont(font);
        scoreText.setString("Pontos: " + to_string(pontuacao));
        scoreText.setCharacterSize(32);
        scoreText.setFillColor(sf::Color::Black);
        recordeText.setString("Recorde: " + to_string(recorde));

        if (watermelonRush) {
            int segundosRestantes = static_cast<int>(ceil(WATERMELON_RUSH_DURATION - watermelonRushClock.getElapsedTime().asSeconds()));
            estadoPowerupText.setString("MELANCIA: " + to_string(max(0, segundosRestantes)) + "s");
        }
        else {
            estadoPowerupText.setString("");
        }

        placarFinalText.setString("Pontos: " + to_string(pontuacao) + "   Recorde: " + to_string(recorde));
        centralizarOrigem(placarFinalText);


        // --- Renderização (Desenho na Tela) ---

        // Usa a máquina de estados para decidir o que desenhar
        switch (currentState) {
        case MENU: {
            jogo.draw(menuBackground);
            jogo.draw(titulo);
            jogo.draw(botaoIniciar);
            jogo.draw(botaoSair);
            jogo.draw(botaoConfig);
            if (showpointer)
                jogo.draw(pointer);
            break;
        }
        case ESCOLHA_DIFICULDADE: {
            jogo.draw(menuBackground); // Mesmo fundo do menu
            jogo.draw(tituloDificuldade);
            for (const auto& bolinha : bolinhasDificuldade) {
                jogo.draw(bolinha);
            }
            break;
        }
        case INICIANDO: {
            jogo.clear(sf::Color(0x000000FF)); // Fundo preto
            loadingSprite.setTexture(Loading[currentLoadingFrame], true);
            loadingSprite.setPosition(sf::Vector2f(0, 0));
            jogo.draw(loadingSprite);
            break;
        }
        case CONFIGURACOES:
        case JOGANDO:
        case PAUSADO:
        case TELA_FIM_DE_JOGO:
        case VITORIA:
        case COMENDO_CROC:
        case GAME_OVER: { // A renderização para JOGANDO e GAME_OVER é muito similar
            // 1. Limpa a tela com a cor de fundo do gramado
            jogo.clear(sf::Color(0x89ac45FF));

            // 2. Desenha o mapa (chão e paredes)
            for (int i = 0; i < ALT; i++) {
                for (int j = 0; j < LAR; j++) {
                    if (mapa[i][j] == '1') {
                        spriteParede.setPosition(sf::Vector2f(j * SIZE, i * SIZE));
                        jogo.draw(spriteParede);
                    }
                    else {
                        // --- Lógica de Autotiling para o Chão ---
                        // Determina qual textura usar verificando os vizinhos da célula
                        string tipo = mapa[i][j] == '2' ? "rio" : "caminho";
                        bool c = i > 0 && mapa[i - 1][j] == '1'; // Vizinho de cima é parede?
                        bool b = i < 12 && mapa[i + 1][j] == '1';// Vizinho de baixo é parede?
                        bool e = j > 0 && mapa[i][j - 1] == '1'; // Vizinho da esquerda é parede?
                        bool d = j < 24 && mapa[i][j + 1] == '1';// Vizinho da direita é parede?
                        int n = c + b + e + d; // Conta o número de paredes vizinhas
                        string chave = tipo + "0"; // Chave padrão

                        // Constrói a chave da textura com base nos vizinhos
                        if (n == 1) chave = tipo + "3-" + (c ? "cim" : b ? "bai" : e ? "esq" : "dir");
                        else if (n == 2) {
                            if (c && b) chave = tipo + "2-hor";
                            else if (e && d) chave = tipo + "2-ver";
                            else if (e && c) chave = tipo + "2-canto-dir-bai";
                            else if (d && c) chave = tipo + "2-canto-esq-bai";
                            else if (e && b) chave = tipo + "2-canto-dir-cim";
                            else if (d && b) chave = tipo + "2-canto-esq-cim";
                        }
                        else if (n == 3) chave = tipo + "1-" + ((!c) ? "cim" : (!b) ? "bai" : (!e) ? "esq" : "dir");
                        else if (n == 4) chave = tipo + "4";

                        if (texturasChao.count(chave)) {
                            spriteChao.setTexture(texturasChao[chave], true);
                            spriteChao.setPosition(sf::Vector2f(j * SIZE, i * SIZE));
                            jogo.draw(spriteChao);
                        }
                        // Desenha a folha, se ainda existir na célula
                        if (mapaFolhas[i][j] && mapa[i][j] != '4') {
                            spriteAsset.setTexture(Assets["folha"], true);
                            spriteAsset.setPosition(sf::Vector2f(j * SIZE, i * SIZE));
                            jogo.draw(spriteAsset);
                        }
                        // Desenha a melancia
                        if (mapaFolhas[i][j] && mapa[i][j] == '4') {
                            spriteAsset.setTexture(Assets["melancia"], true);
                            spriteAsset.setPosition(sf::Vector2f(j * SIZE, i * SIZE));
                            jogo.draw(spriteAsset);
                        }
                    }
                }
            }

            // 3. Desenha os personagens
            if (currentState != GAME_OVER && currentState != COMENDO_CROC) {
                // Atualiza a textura do jogador (terra ou água)
                if (pacNaAgua)
                    jogador.Sprite.setTexture(Texturasswim[jogador.ultimadir][jogador.animationFrame], true);
                else
                    jogador.Sprite.setTexture(Texturas[jogador.ultimadir][jogador.animationFrame], true);

                jogador.Sprite.setPosition(jogador.visualPos);
                jogo.draw(jogador.Sprite);
            }

            // Desenha todos os fantasmas
            for (Ghost& croc : ListaFantasmas) {
                // No estado GAME_OVER, não desenha o fantasma que está na animação de colisão
                if (currentState == GAME_OVER && &croc == crocColidido) continue;
                if (currentState == COMENDO_CROC && &croc == crocComido) continue;

                bool crocNaAgua = estaNaAgua(croc.visualPos);
                const auto& texCroc = croc.isWeak
                    ? (crocNaAgua ? croc.animWeakSwim[croc.dir] : croc.animWeak[croc.dir])
                    : (crocNaAgua ? croc.animSwim[croc.dir] : croc.animNormal[croc.dir]);
                croc.sprite.setTexture(texCroc[croc.animFrame], true);

                croc.sprite.setColor(sf::Color::White);
                croc.sprite.setPosition(croc.visualPos);
                jogo.draw(croc.sprite);
            }

            // Se for GAME_OVER, desenha a animação de morte por cima
            if (currentState == GAME_OVER) {
                sf::Sprite spriteAnimacao(placeholderTexture);
                spriteAnimacao.setTexture(GameOver[dirAnimGameOver][frameAnimGameOver], true);
                spriteAnimacao.setPosition(posAnimGameOver);
                jogo.draw(spriteAnimacao);
            }
            if (currentState == COMENDO_CROC) {
                sf::Sprite spriteAnimacao(placeholderTexture);
                int frameSeguro = min(frameAnimEatCroc, CAP_EAT_CROC_FRAME_COUNT - 1);
                spriteAnimacao.setTexture(CapEatCroc[dirAnimEatCroc][frameSeguro], true);
                spriteAnimacao.setPosition(posAnimEatCroc);
                jogo.draw(spriteAnimacao);
            }

            // 4. Desenha a Interface do Usuário
            sf::RectangleShape barralat(sf::Vector2f(UI_PANEL_WIDTH, WINDOW_HEIGHT));
            sf::RectangleShape barrainf(sf::Vector2f(WINDOW_WIDTH, UI_BAR_HEIGHT));
            barralat.setPosition(sf::Vector2f(MAP_WIDTH, 0));
            barrainf.setPosition(sf::Vector2f(0, MAP_HEIGHT));
            barralat.setFillColor(sf::Color(0, 180, 255));
            barrainf.setFillColor(sf::Color(0, 180, 255));
            jogo.draw(barralat);
            jogo.draw(barrainf);

            scoreText.setPosition(sf::Vector2f(MAP_WIDTH - 390, MAP_HEIGHT + 10));
            jogo.draw(scoreText);
            jogo.draw(recordeText);
            if (watermelonRush) {
                jogo.draw(estadoPowerupText);
            }

            sf::Sprite life(placeholderTexture);
            sf::Vector2f lifepos(MAP_WIDTH + 5, 30);
            for (int i = 0; i < Life.size(); i++) {
                if (Life[i]) {
                    life.setTexture(Assets["full-heart"], true);
                }
                else {
                    life.setTexture(Assets["empty-heart"], true);
                }
                life.setPosition(lifepos);
                lifepos.y += 50;
                jogo.draw(life);
            }
            if (melan[0]) {
                life.setTexture(Assets["blue-heart"], true);
                life.setPosition(lifepos);
                lifepos.y += 50;
                jogo.draw(life); 
            }
            if (melan[1]) {
                life.setTexture(Assets["blue-heart"], true);
                life.setPosition(lifepos);
                jogo.draw(life); 
            }

            if (currentState == PAUSADO) {
                jogo.draw(fundoEsc);
                jogo.draw(textoPause);
                jogo.draw(botaoConfig);
                if (showpointer)
                    jogo.draw(pointer);
            }

            if (currentState == CONFIGURACOES) {
                if (prevState == MENU) {
                    jogo.draw(menuBackground);
                }
                else if (prevState == PAUSADO) {
                }

                jogo.draw(fundoEsc);
                jogo.draw(tituloConfig);
                jogo.draw(labelVolume);
                jogo.draw(volumeTrack);
                jogo.draw(volumeHandle);
                jogo.draw(valorVolumeText);
                jogo.draw(botaoVoltar);
                if (showpointer)
                    jogo.draw(pointer);
            }

            if (currentState == TELA_FIM_DE_JOGO) {
                jogo.draw(fundoEsc);

                // Desenha os textos e botões da tela de Fim de Jogo
                jogo.draw(textoFimDeJogo);
                jogo.draw(placarFinalText);
                jogo.draw(botaoJogarNovamente);
                jogo.draw(botaoVoltarMenu);
                if (showpointer)
                    jogo.draw(pointer);
            }

            if (currentState == VITORIA) {
                vitoriaSprite.setTexture(animacaoVitoria[frameVitoria], true);
                vitoriaSprite.setPosition(sf::Vector2f(0, 0));
                jogo.draw(vitoriaSprite);

                jogo.draw(textoVitoria);
                jogo.draw(placarFinalText);
                jogo.draw(botaoJogarNovamente);
                jogo.draw(botaoVoltarMenu);
                if (showpointer)
                    jogo.draw(pointer);
            }

            break;
        }
        }

        // 5. Exibe tudo o que foi desenhado na janela
        jogo.display();
    }

    return 0;
}
