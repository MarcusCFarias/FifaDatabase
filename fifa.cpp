#include "parser.hpp"		//Link para o Parser

//BIBLIOTECAS NATIVAS UTILIZADAS------------------------------------------------------------------------------------------------------

#include <stdexcept>		
#include <chrono>
#include <cctype>
#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>

//DEFINICAO DE CONSTANTES DE NAMESPACES-----------------------------------------------------------------------------------------------

const int SIZE = 8000;
const int SIZE_TAGS = 12000;
const int SIZE_TAGS_2 = 500;
const int SIZEUSERS = 8000;

const int ALPHABET_SIZE = 26;

using namespace aria::csv;
using namespace std;


//-----------------------------------------------------------------------------------------------------------------------------
//ÁREA RESERVADA AOS STRUCTS --------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

struct jogadores { 			//Tabela_Jogadores com os dados de jogadores
	int id;					//ID do jogador	
	string name;			//Nome do jogador	
	string position;		//Posicao do jogador
	int count;				//Contagem de quantas avaliacoes o jogador tem
	float rating;			//Rating global do jogador
	vector<string> flags;	//Vetor de flags para parte das tags (ver seção da definição de classes)
	jogadores* next;		//Ponteiro para o próximo jogador, caso haja colisão na tabela
};

struct Tags { 				//Struct para a Tabela_Jogadores das tags (Tabela_Jogadores principal de tags)
	int usuarios;			//ID dos usuarios que avaliaram um jogador com alguma tag
	int fifaid_jogador;		//ID do jogador que tem a TAG citada no struct
	string etiqueta;		//Etiqueta representa o nome da TAG 
	Tags* next;				//Ponteiro para o próximo valor, caso haja colisão
};

struct Tag_map { 			//Struct que serve para a criação do Hashmap das tags
	int id;					//ID do jogador
	int value = 0;			//Valor (começa setado em zero)
	Tag_map* next;			//Ponteiro para o próximo elemento do hashmap
};

struct Usuarios { 								//Struct para a Tabela_Jogadores dos usuários
	int id;										//ID do usuario
	vector<pair<int, float>> players_info;		//Vetor em par para guardar um inteiro para o ID do jogador avaliado (int) e nota do usuario (float)
	Usuarios* next;								//Ponteiro para o próximo nodo da Hash de usuarios (Caso haja alguma colisão)
};

struct Nodo_Trie         						//Struct para os nodos da arvore Trie
{
	struct Nodo_Trie* filhos[ALPHABET_SIZE];	//Filhos no nodo pai (São n filhos, um para cada letra representada na Trie)
	bool EhPalavra;								//Booleano que diz se é ou não o fim de uma palavra
	string name;								//Nome do jogador (Foi utilizado somente para casos de teste, no fim das contas, é possível tirar esse parametro)
	int id = -1;        					    //Caso o booleano for ligado, salva o ID do usuario em "id"
};

//----------------------------------------------------------------------------------------------------------------------------------
//FUNCOES DE HASH-------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------

int Funcao_Hash_Nao_Tag(int id, int tamanho) {			//Funcao de Hash para Tabela_Jogadoress que não são a de tags(usuarios e jogadores)

	int position_hash;									//Position_hash recebe o módulo do ID pelo Tamanho
	position_hash = id % tamanho;
	return position_hash;								//Retorna a position_hash
}

int Funcao_Hash_Tags(string tag, int tamanho) {	//Funcao de hash especial para os Tags

	int identificador = 0;						//Identificador para a criacao de hash das tags
	int position_hash;							//Position hash vai receber o modulo do identificador pelo tamanho


	for (char letra : tag) { 					//Soma os asciis da Tag para criar uma funcao de hash para cada e, assim, evitar muitas colisoes
		identificador += letra;
	}

	position_hash = identificador % tamanho;

	return position_hash;						//Retorna o position_hash (localizacao na Tabela_Jogadores)
}



//----------------------------------------------------------------------------------------------------------------------------------
//AREAS DESTINADAS AS CLASSES DO PROGRAMA (NAO ENGLOBA AS CLASSES DE USUARIOS)------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------

class Tabela_Jogadores {								//Classe das tabelas de jogadores
public:

	Tabela_Jogadores(int size) :tamanho(size) {   		//Dita o tamanho da class

		Tabela_JogadoresVector.resize(tamanho, nullptr); //Linha que reajusta o tamanho dos elementos
	}



	void Inserir(int id, string name, string position) {	//Funcao para a insercao de elementos na hash de jogadores
		int i;
		i = Funcao_Hash_Nao_Tag(id, tamanho);				//i Recebe a funcao de hash de jogadores para determinar a posicao na hash
		struct jogadores* novo_nodo = new struct jogadores; //Aloca um novo nodo
		novo_nodo->id = id;									//Coloca todos os parametros dentro do novo nodo alocado
		novo_nodo->name = name;
		novo_nodo->position = position;
		novo_nodo->next = Tabela_JogadoresVector[i]; 		//Como o novo usuario eh alocado na frente da hashlist, o parametro next recebe o inicio da hashlist
		Tabela_JogadoresVector[i] = novo_nodo;    			//Logo depois, coloca na tabela o novo nodo devidamente alocado
	}

	struct jogadores* Insere_Ratings(int id, float rating, int count) {		//Operacao para a insercao dos ratings na hash de jogadores
		int i;
		i = Funcao_Hash_Nao_Tag(id, tamanho);					//Pega a posicao com a funcao de hash
		struct jogadores* ptaux = Tabela_JogadoresVector[i];	//Cria um nodo auxiliar para travessia na tabela

		while (ptaux != nullptr) {								//Operacao padrao de insercao de ratings (similar a "Inserir" -> ver primeira funcao da classe)
			if (ptaux->id == id) {
				ptaux->count = count;
				ptaux->rating = rating;
				return nullptr;									//Retorna ponteiro nulo caso chegue no final
			}

			ptaux = ptaux->next;
		}

		return nullptr;											//Retorna ponteiro nulo caso chegue no final
	}

	struct jogadores* Procura(int id) {						//Procura a existencia de um dado ID na hash de jogadores
		int i;
		i = Funcao_Hash_Nao_Tag(id, tamanho);				//Usa o hash para descobrir a posicao do ID no vetor
		struct jogadores* ptaux = Tabela_JogadoresVector[i];	//Ponteiro auxiliar para travessia na hash

		while (ptaux != nullptr) {							//Operacao padrao de caminhada em listas simplesmente encadeadas
			if (ptaux->id == id) {
				return ptaux;								//Caso ache
			}

			ptaux = ptaux->next;
		}

		return NULL;										//Caso nao ache
	}



	void Insere_Jogador_Count_1K(vector<pair<int, float>>& arr, string position) {		//Funcao para buscar topN jogadores

		struct jogadores* ptaux;
		int id;
		float rating;
		string posit;
		size_t achou;
		for (size_t i = 0; i < Tabela_JogadoresVector.size(); i++)					//Faz o tratamento para verificar se COUNT >= 1000
		{	
			ptaux = Tabela_JogadoresVector[i];

			while (ptaux != nullptr)											//Verifica se o jogador contem a string crrespondente
			{
				posit = ptaux->position;

				achou = posit.find(position);


				if (achou != string::npos && ptaux->count >= 1000) {
					id = ptaux->id;
					rating = ptaux->rating;

					arr.push_back(make_pair(id, rating));
				}


				ptaux = ptaux->next;

			}
		}
	}

	~Tabela_Jogadores() {							//Destrutor da classe Tabela_Jogadores (Todo o resto é bem padrao para a destruicao)
		for (int i = 0; i < tamanho; i++) {
			jogadores* ptaux = Tabela_JogadoresVector[i]; 

			while (ptaux != NULL) {
				jogadores* next = ptaux->next;
				delete(ptaux);
				ptaux = next;
			}
		}
	}

private:
	int tamanho;
	vector<jogadores*> Tabela_JogadoresVector;
};


class Tabela_JogadoresTags {				//A classe de Tags segue exatamente a mesma lógica da class anterior
public:

	Tabela_JogadoresTags(int size) :tamanhoTags(size) {   //Parte ja explicada na parte das funcoes
		Tabela_JogadoresVectorTags.resize(tamanhoTags, nullptr);
	}


	void InserirTag(int user_id, int id, string tag) {		//Exatamente o mesmo metodo de insercao
		int i;
		i = Funcao_Hash_Tags(tag, tamanhoTags);
		struct Tags* novo_nodo = new struct Tags;
		novo_nodo->usuarios = user_id;
		novo_nodo->fifaid_jogador = id;
		novo_nodo->etiqueta = tag;
		novo_nodo->next = Tabela_JogadoresVectorTags[i];
		Tabela_JogadoresVectorTags[i] = novo_nodo;
	}

	vector<int> ProcuraTag(string tag) { //Mesmo metodo de procura da funcao "Procura" da class "Tabela_Jogadores"

		vector<int> intersect;
		int i;
		int currentSofifa_id = -1;
		i = Funcao_Hash_Tags(tag, tamanhoTags);
		struct Tags* ptaux = Tabela_JogadoresVectorTags[i];

		while (ptaux != nullptr) {								//Percorre a lista encadeada da hash buscando pela tag
			if (ptaux->etiqueta == tag) {
				if (currentSofifa_id != ptaux->fifaid_jogador) {
					intersect.push_back(ptaux->fifaid_jogador);
					currentSofifa_id = ptaux->fifaid_jogador;

				}

			}

			ptaux = ptaux->next;
		}

		return intersect;
	}

	~Tabela_JogadoresTags() {							//Destrutor da classe Table_JogadoresTags(procedimento padrao)
		for (int i = 0; i < tamanhoTags; i++) {
			Tags* ptaux = Tabela_JogadoresVectorTags[i]; 

			while (ptaux != NULL) {
				Tags* next = ptaux->next; 
				delete(ptaux);
				ptaux = next;
			}
		}
	}

private:
	int tamanhoTags;
	vector<Tags*> Tabela_JogadoresVectorTags;
};




class Tabela_JogadoresTag_map {						//Classe para o segundo struct das tags (serve para a parte de hashmap)
public:
	Tabela_JogadoresTag_map(int size) :sizeTag_map(size) {		//Inicializa tamanho
		hashTag_mapVector.resize(sizeTag_map, nullptr);
	}

	void Insercao_map(int id) {						//Novamente, é basicamente a mesma ideia das outras funcoes de insercao das duas classes anteriores
		int i;
		i = Funcao_Hash_Nao_Tag(id, sizeTag_map);	
		Tag_map* novo_nodo = new Tag_map;
		novo_nodo->id = id;
		novo_nodo->value = 1;
		novo_nodo->next = hashTag_mapVector[i];
		hashTag_mapVector[i] = novo_nodo;
	}

	int Procura_map(int id) { 				//Passa os dados entre as hashs (Se ja existir nodo, aumenta o value)
		int i;
		i = Funcao_Hash_Nao_Tag(id, sizeTag_map);
		Tag_map* ptaux = hashTag_mapVector[i];

		while (ptaux != NULL) {				//Mesma operacao de passar pela LSE

			if (ptaux->id == id) {
				ptaux->value += 1; 			//A unica diferenca eh que o nodo aumenta de value de ja existir um nodo com mesmo id
				return 1; 
			}

			ptaux = ptaux->next;
		}
		return 0;
	}


	vector<int> Procura_interseccao(int cardinalidade) { //Procura os ids em qe ha interseccao de tags (A funcao funciona de forma tag - tag, ou seja , duas tags por vez) 
		vector<int> vetor_de_ids;

		Tag_map* ptaux = hashTag_mapVector[0];

		for (int i = 0; i < hashTag_mapVector.size(); i++) { //Precisa passar por toda a hash, infelizmente. Seria mais rapido se fosse
			ptaux = hashTag_mapVector[i];					//Utilizada uma trie
			while (ptaux != nullptr) {
				if (ptaux->value == cardinalidade) {
					vetor_de_ids.push_back(ptaux->id);
				}
				ptaux = ptaux->next;
			}
		}

		return vetor_de_ids;
	}

	~Tabela_JogadoresTag_map() {					//Destrutor da classe Tabela_JogadoresTag_map
		for (int i = 0; i < sizeTag_map; i++) {
			Tag_map* ptaux = hashTag_mapVector[i]; 

			while (ptaux != NULL) {
				Tag_map* next = ptaux->next; 
				delete(ptaux);
				ptaux = next;
			}
		}
	}

private:
	int sizeTag_map;
	vector<Tag_map*> hashTag_mapVector;

};

//-----------------------------------------------------------------------------------------------------------------------------
//FUNCOES PARA VALIDACAO ------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

int Validacao_de_Top(string comando) {		//Função booleana para verificar se a string digitada eh equivalente ao "topN"

	int booleano_top = 1;								//A variavel comeca em "true", representado por 1

	if (comando.length() <= 3)
		booleano_top = 0;								//Se tiver menos de 3 caracteres coloca a variavel em 0 -->  "false"
	else
		if (toupper(comando.at(0)) != 'T'
			|| toupper(comando.at(1)) != 'O'
			|| toupper(comando.at(2)) != 'P')
			booleano_top = 0;							//Se os caracteres forem diferentes de "TOP" coloca o booleano em 0 --> "false"

	return booleano_top;
}


int Validacao_de_Argumentos_Top(string argumentos) {	//Validacao dos outros argumentos (N 'pos')

	int booleano_top = 1;					//Comeca com o booleano em "true" --> 1

	if (argumentos.length() < 3)
		booleano_top = 0;					//Se o tamanho dos argumentos for menor do que 3, coloca booleano em "false" --> 0
	else if (argumentos.at(0) != '\''
		|| argumentos.at(argumentos.length() - 1) != '\'')		//Verifica se o segundo argumento está entre aspas simples
		booleano_top = 0;					//Se nao estiver, coloca o booleano em 0 --> "false"

	return booleano_top;					//Retorna o booleano
}

int Retrieve_N_Argument(string comando) {	//Retorna o valor N da funcao top (topN 'pos')

	int valor_final;						//Valor que recebe a string convertida
	string str_num;

	int posicao = 3;						
	while (comando.size() != posicao)		//Copia "comando" para str_num
	{
		str_num += comando[posicao];

		posicao++;
	}

	valor_final = stoi(str_num);			//Transfoma a string em int usando a funcao "stoi"
	return valor_final;
}

string Retrieve_Pos_Argument(string argumento) {	//Retorna a posicao entre aspas simples

	string str;							//String que vai ser devolvida com a posicao no final da verificacao

	int point = 1;						//Comeca apontando para a posicao 1
	while (argumento.size() - 1 != point)		//Retira as aspas e devolve a string sem aspas
	{
		str += argumento[point];
		point++;
	}

	for (size_t i = 0; i < str.size(); i++) {
		str[i] = toupper(str[i]);

	}


	return str;							//Retorno da funcao
}

//-----------------------------------------------------------------------------------------------------------------------------
//FUNCOES PARA QUICKSORT ------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

static int Random(int min, int max)						//A partir daqui, sao apenas as funcoes para o quicksort, 
{														//todas foram retiradas do laboratorio dois do Marcus, apenas um pouco modificadas
	random_device rd;
	mt19937 gen(rd());

	uniform_int_distribution<> dis(min, max);

	int randomNumber = dis(gen);

	return randomNumber;
}
static void Swap(vector<pair<int, float>>& a, int i, int j)
{
	int tempId = a[i].first;
	float tempRating = a[i].second;

	a[i].first = a[j].first;
	a[i].second = a[j].second;

	a[j].first = tempId;
	a[j].second = tempRating;
}

static int Partition_Lomuto(vector<pair<int, float>>& a, int left, int right)
{
	float pivot = a[left].second;
	int storeIndex = left + 1;

	for (int i = left + 1; i <= right; i++)
	{
		float nextElemet = a[i].second;
		if (nextElemet > pivot)
		{
			if (i != storeIndex)
				Swap(a, i, storeIndex);
			storeIndex++;
		}
	}

	Swap(a, left, storeIndex - 1);

	return storeIndex - 1;
}

static void QuickSort_Random_Lomuto(vector<pair<int, float>>& a, int left, int right)
{
	int pivot, randomNumber;

	if (left < right)
	{
		randomNumber = Random(left, right);
		Swap(a, left, randomNumber);

		pivot = Partition_Lomuto(a, left, right);

		QuickSort_Random_Lomuto(a, left, pivot - 1);
		QuickSort_Random_Lomuto(a, pivot + 1, right);
	}
}


//-----------------------------------------------------------------------------------------------------------------------------
//FUNCAO PARA BUSCAR OS TOPS --------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
void Busca_de_Top_Players(vector<string> param, Tabela_Jogadores* Hash_players_busca) {		//Procura os players e printa eles na tela

	string comando = param[0];
	string argumento = param[1];

	int numero = Retrieve_N_Argument(comando);
	string posicao = Retrieve_Pos_Argument(argumento);

	vector<pair<int, float>> arrJogadorRating;

	Hash_players_busca->Insere_Jogador_Count_1K(arrJogadorRating, posicao);			//Verificacao de players com mais de 1000 counts

	if (arrJogadorRating.size() > 0) {

		cout << "\n" << endl;								//Quebra de linha

		QuickSort_Random_Lomuto(arrJogadorRating, 0, arrJogadorRating.size() - 1);		//Chama o quicksort para ordenar o array de players com mais de 1000 de count

		printf("FIFA_ID			NOME		POS			RATING			COUNT\n\n");
		for (size_t i = 0; i < numero; i++)	//Laco para procurar os jogadores
		{
			jogadores* jgd = Hash_players_busca->Procura(arrJogadorRating[i].first);

			cout << jgd->id << " " << jgd->name << " " << jgd->position << " " << jgd->rating << " " << jgd->count << endl;
		}
	}
}

//-----------------------------------------------------------------------------------------------------------------------------
//FUNCOES PARA A TRIE --------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

struct Nodo_Trie* getNode(void)						//Simplesmente aloca um nodo novo para a trie (Parte deste codiog foi retirado do exemplo do proprio prfessor)
{													//Prinicpalmente esta funcao e a de Insercao
	struct Nodo_Trie* pNode = new Nodo_Trie;		//Aloca o nodo

	pNode->EhPalavra = false;						//Coloca o booleano da palavra em "false"
	pNode->name = "NULL";							//Coloca o "name" em NULL	

	for (int i = 0; i < ALPHABET_SIZE; i++)			//Povoa os nodos filhos com NULL
		pNode->filhos[i] = NULL;

	return pNode;									//Retorna o nodo alocado
}

void insert(struct Nodo_Trie* root, string key, int id)			//Insercao na trie
{

	struct Nodo_Trie* pCrawl = root;				//Nodo auxiliar "pCrawl" para caminhar pela trie

	for (int i = 0; i < key.length(); i++)			//Ate o fim do tamanho da palavra, repetir o laco
	{
		if (key[i] == ' ' || key[i] == '.' || key[i] == '-' || key[i] == 96 || key[i] == 180 || key[i] == 145 || key[i] == 146 || key[i] == 39) {
			//Se forem estes carcateres, ignora
		}
		else {
			int index = key[i] - 'a';				//Se nao, ve qual o nodo corrrespondente na trie fazendo (char - 'a')
			if (!pCrawl->filhos[index])
				pCrawl->filhos[index] = getNode();

			pCrawl = pCrawl->filhos[index];
		}
	}

	pCrawl->EhPalavra = true;			//Quando chega no final, liga o booleano de "ehpalavra"
	pCrawl->name = key;					//Nome recebe a chave que foi passada (string com o nome, no caso)
	pCrawl->id = id;					//ID recebe o ID do jogador
	return;
}

void traverse(struct Nodo_Trie* root, Tabela_Jogadores& table) {		//Basicamente uma funcao recursiva que passa por todos os nodos 
																		//Subjacentes ao do prefixo dado
	if (root->EhPalavra) {
		jogadores* aux;
		aux = table.Procura(root->id);									//Usa a funcao de procura, tendo o ID
		std::cout << " " << aux->id << " " << aux->name << " " << aux->position << " " << aux->rating << " " << aux->count << std::endl;
		return;
	}


	for (int index = 0; index < ALPHABET_SIZE; index++) {				//Laco for para cada nodo filho, retorna se passar for todos
		struct Nodo_Trie* pChild = root->filhos[index];
		if (pChild != NULL) {
			traverse(pChild, table);
		}
	}
	return;
}

void print_trie_prefix(string prefix, struct Nodo_Trie* root, Tabela_Jogadores* table) {		//Inicio da funcao de print da trie
	struct Nodo_Trie* pCrawl = root;
	for (int i = 0; i < prefix.length(); i++) {
		if (prefix[i] == ' ' || prefix[i] == '.' || prefix[i] == '-' || prefix[i] == 96 || prefix[i] == 180 || prefix[i] == 145 || prefix[i] == 146 || prefix[i] == 39) {

		}
		else {
			int index = prefix[i] - 'a';					//Caminha pela arvore ate chegar no final do prefixo
			if (!pCrawl->filhos[index]) {
				cout << "Prefixo nao Existe" << std::endl;	//Se nao existir, escreve "prefixo nao existe"
				return;
			}
			pCrawl = pCrawl->filhos[index];		
		}
	}
	printf("FIFA_ID			NOME		POS			RATING			COUNT\n\n");	//Printa o cabecalho
	traverse(pCrawl, *table);							//Chama a recursao

	return;
}

//-----------------------------------------------------------------------------------------------------------------------------
//FUNCOES E CLASSES PARA OS USUSARIOS -----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

class Tabela_Usuarios {							//Classe para a hashtable dos usuarios
public:

	Tabela_Usuarios(int size) :tamanho(size) {   //Define o tamanho da hash

		Tabela_JogadoresVectorUsers.resize(tamanho, nullptr); //Da resize e atribui a todos os valores "ptrnull"
	}

	void Inserir_Usuarios(int user, int player_id, float rating) {		//Operacao padrao de insercao, muito parecida com as anteriores
		int i;
		i = Funcao_Hash_Nao_Tag(user, tamanho);
		Usuarios* novo_nodo = new Usuarios;
		novo_nodo->id = user;
		novo_nodo->players_info.push_back(std::make_pair(player_id, rating));
		novo_nodo->next = Tabela_JogadoresVectorUsers[i]; 
		Tabela_JogadoresVectorUsers[i] = novo_nodo;    
	}

	struct Usuarios* Procura_Usuarios(int id) {						//Funcao que retorna o nodo de um usuario especifico
		int i;
		i = Funcao_Hash_Nao_Tag(id, tamanho);						//Chama a funcao de hash basica
		struct Usuarios* ptaux = Tabela_JogadoresVectorUsers[i];		//Aloca memoria

		while (ptaux != nullptr) {									//Travessia pelo vetor, tambem muito igual as anteriores
			if (ptaux->id == id) {
				return ptaux;
			}

			ptaux = ptaux->next;
		}

		return  nullptr;
	}
	

	struct Usuarios* Inserir_Jogadores_Usuarios(int user, int id, float rating) {	//Insere os jogadores cujos usuarios deram nota
		unsigned int index;
		index = Funcao_Hash_Nao_Tag(user, tamanho);
		Usuarios* ptaux = Tabela_JogadoresVectorUsers[index];

		while (ptaux != nullptr) {
			if (ptaux->id == user) {
				ptaux->players_info.push_back(std::make_pair(id, rating));		//Anda ate o usuario e da um push.back no vetor com os dados de id e nota
				return nullptr;
			}

			ptaux = ptaux->next;
		}

		return nullptr;
	}

	int Imprimir_Usuarios(int id, Tabela_Jogadores* table) {	//Pega todos os jogadores, joga em um vetor e ordena eles, depois printa na tela
		int in;
		in = Funcao_Hash_Nao_Tag(id, tamanho);
		Usuarios* ptaux = Tabela_JogadoresVectorUsers[in];
		while (ptaux != nullptr) {
			if (ptaux->id == id) {

				QuickSort_Random_Lomuto(ptaux->players_info, 0, ptaux->players_info.size() - 1);	//Chama o quicksort para ordenar
				cout << ptaux->players_info.size();
				struct jogadores* aux;
				
				printf("FIFA_ID			NOME		POS			RATING			COUNT		NOTA\n\n");
				for (size_t i = 0; (i < 20) && (i < ptaux->players_info.size()); i++) { 	//Laco for para cobrir todos os jogadores
					aux = table->Procura(ptaux->players_info[i].first); 					//Procura os IDs.
					std::cout << " " << aux->id << " " << aux->name << " " << aux->position <<" " << aux->rating << " " << aux->count << " " << ptaux->players_info[i].second << std::endl;
				}
				return 0; 
			}
			else {
				ptaux = ptaux->next;
			}


		}
		return 0; //retorna 0 
	}



	~Tabela_Usuarios() {					//Destructor da classe Tabela_Usuarios (igual a todos os outros)
		for (int i = 0; i < tamanho; i++) {
			Usuarios* ptaux = Tabela_JogadoresVectorUsers[i]; 

			while (ptaux != NULL) {
				Usuarios* next = ptaux->next; 
				delete(ptaux);
				ptaux = next;
			}
		}
	}

private:
	int tamanho;
	vector<Usuarios*> Tabela_JogadoresVectorUsers;
};

//-----------------------------------------------------------------------------------------------------------------------------
//TOKENIZACAO -----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

vector<string> LerArgumentos(string entrada) { 		//Recebe uma entrada com todas as tags em linha

	vector<string> n_tags;

	size_t first = entrada.find('\'');		//Verifica onde estao os apostrofos
	size_t last;

	while (first != std::string::npos) {
		last = entrada.find('\'', first + 1);			//Encontra o apostrofo
		if (last == std::string::npos) {
			break;
		}

		string novaString = entrada.substr(first + 1, last - first - 1);		
		n_tags.push_back(novaString);				//Retorna todas as variaveis de tags no vetor n_tags

		first = entrada.find('\'', last + 1);
	}

	return n_tags;									//Retorno
}

void isPlayer(Nodo_Trie* jogadores, Tabela_Jogadores* table, vector<string> tokens) {	//Se chamar o jogador, verifica qual o prefixo e chama a print_trie_prefix


	if (tokens.size() == 2) {				//Transforma todos os valores da string em minusculo
		for (char& c : tokens[1]) {
			c = std::tolower(c);
		}
	}
	print_trie_prefix(tokens[1], jogadores, table);				//Funcao print para a trie ja explicada anteriormente
}


void isUser(Tabela_Usuarios* tableUser, Tabela_Jogadores* table, vector<string> tokens) {
	if (tokens.size() == 2)
		tableUser->Imprimir_Usuarios(stoi(tokens[1]), table);		//Simplesmente chama a funcao de print de ususarios ja explicada enteriormente
	else
		cout << "Parametros Invalidos" << std::endl;
}


void isTop(Tabela_Jogadores* table, vector<string> tokens) {

	if (Validacao_de_Top(tokens[0]) == 1 && tokens.size() == 2) {

		if (Validacao_de_Argumentos_Top(tokens[1]) == 1) {
			Busca_de_Top_Players(tokens, table);				//Assim como a anterior, simplesmente chama a funcao ja feita anteriormente


		}
		else
			cout << "Parametros Invalidos" << std::endl;
	}
	else
		cout << "Parametros Invalidos" << std::endl;
}

//-----------------------------------------------------------------------------------------------------------------------------
//MAIN -----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------


int main(void) {

	chrono::steady_clock relogio;		//Inicializa o relogio
	auto start = relogio.now();			//Comeca a contar

	Tabela_Jogadores table(SIZE);			//Inicializa uma tabela com tamanho definido no inicio do programa
	Tabela_Usuarios tableUser(SIZEUSERS);		//Inicializa uma tabela com tamanho definido no inicio do programa

	std::ifstream players("players.csv");		//Abre o arquivo "players.csv"


//-----------------------------------------------------------------------------------------------------------------------------
//ARQUIVO PLAYERS --------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

	CsvParser parserP(players); //Esta funcao esta listada na biblioteca disponibilizada pelo professor, a parte de leitura so foi retirada de la com algumas leves mudancas

	std::string linhaP;  

	std::getline(players, linhaP);

	std::string pos_player;
	std::string name_player;
	int ids_player;
	std::string aux = name_player;
	int i;
	struct Nodo_Trie* jogadores = getNode();
	std::vector<std::string> linha_dados; 

	for (auto& row : parserP) {
		linha_dados.clear();
		for (auto& field : row) {
			linha_dados.push_back(field);
		}
		ids_player = std::stoi(linha_dados[0]); 
		name_player = linha_dados[1];
		pos_player = linha_dados[2];
		table.Inserir(ids_player, name_player, pos_player);		//Usa a funcao de inserir na hash dos players
		aux = name_player;
		for (i = 0; i < aux.length(); i++) {
			if (aux[i] >= 'A' && aux[i] <= 'Z')				//Transforma em minusculo
				aux[i] = aux[i] - ('A' - 'a');
		}
		insert(jogadores, aux, ids_player);					//Insere na trie

	}
	players.close();									//Fecha o arquivo

//-----------------------------------------------------------------------------------------------------------------------------
//ARQUIVO RATING -----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------



	std::ifstream rating("rating.csv");
	std::getline(rating, linhaP);   //Parte para retirar a primeira linha, simplesmente
	CsvParser parserR(rating);

	double  global_sum = 0;					//Essa parte do parser eh um pouco diferente pq ela se aproveita do fato do rating de jogadores 
	int count = 0;							//Estarem divididos em blocos, entao, ele le todos os usuarios que deram nota pra determinado jogador
	float global_rating;					//(Comeca do messi) e vai populando as hashs dessa forma (tanto de players quanto de ususarios)
	string atual = "158023";   
	for (auto& row : parserR) {
		linha_dados.clear();

		for (auto& field : row) { 
			linha_dados.push_back(field);
		}
		if (atual == linha_dados[1]) {
			global_sum += stof(linha_dados[2]);			//Calculo da media global
			count++;
		}
		else {
			global_rating = global_sum / count;
			table.Insere_Ratings(stoi(atual), global_rating, count);	//Insere na tabela de jogadores
			atual = linha_dados[1];
			global_sum = stof(linha_dados[2]);
			count = 1;
		}

		if (tableUser.Procura_Usuarios(stoi(linha_dados[0])) == nullptr) {								//Adiciona usuarios
			tableUser.Inserir_Usuarios(stoi(linha_dados[0]), stoi(linha_dados[1]), stof(linha_dados[2])); 
		}
		else {
			tableUser.Inserir_Jogadores_Usuarios(stoi(linha_dados[0]), stoi(linha_dados[1]), stof(linha_dados[2])); 
		}


	}
	rating.close();				//Fecha o arquivo




//-----------------------------------------------------------------------------------------------------------------------------
//ARQUIVO TAGS ----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

	Tabela_JogadoresTags tableTags(SIZE_TAGS);


	ifstream tags("tags.csv");				//Exatamente a mesma ideia dos anteriores

	CsvParser ParserT(tags); 
	string linhaPtags;

	getline(tags, linhaPtags); 

	int user_id_tag;
	int id_tag;
	string tag_tag;

	vector<std::string> linha_dadost; 

	for (auto& row : ParserT) {
		linha_dadost.clear();
		for (auto& field : row) {
			linha_dadost.push_back(field);
		}
		user_id_tag = stoi(linha_dadost[0]); 
		id_tag = stoi(linha_dadost[1]); 
		tag_tag = linha_dadost[2];
		tableTags.InserirTag(user_id_tag, id_tag, tag_tag);		//Popula as tags
	}
	tags.close();					//Fecha o arquivo de tags

//-----------------------------------------------------------------------------------------------------------------------------
//INTERCAO COM O USUSARIO -----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------

	auto end = relogio.now();		//Desliga o contador
	auto tempo = static_cast<chrono::duration<double>>(end - start);		//Coloca o valor em segundos na variavel tempo
	cout << "Tempo de Execucao:  " << tempo.count() << " segundos" << endl;		//Printa o tempo na tela

	string token;
	string input;

	while (input != "0") {				//Enquanto o usuario nao escrever "0" para sair do programa


		cout << "$ ";					//Escreve a abertura "$ "
		getline(cin, input);			//Pega o input do usuario

		stringstream ss(input);
		vector<string> tokens;
		while (getline(ss, token, ' ')) {	
			tokens.push_back(token);
		}
		for (char& c : tokens[0]) {
			c = std::tolower(c);
		}
		//Trata o input para cada um dos casos
		if (tokens[0] == "user") isUser(&tableUser, &table, tokens);	//S o inicio for user
		if (tokens[0] == "player") isPlayer(jogadores, &table, tokens);	//Se o inicio for player
		if (Validacao_de_Top(tokens[0]) == 1)		//Verifica a validacao se a string inicial eh top
		{
			isTop(&table, tokens);
		}
		if (tokens[0] == "tags") {			//Se o inicio for tag

			Tabela_JogadoresTag_map tableTag_map(SIZE_TAGS_2);

			tokens.clear();				//Junta os tokens de novo

			tokens = LerArgumentos(input);		//Chama a funcao de verificacao


			vector<vector<int>> intersect; //Guarda as interseccoes

			for (int i = 0; i < tokens.size(); i++) {
				intersect.push_back(tableTags.ProcuraTag(tokens[i]));
			}


			for (int i = 0; i < intersect.size(); i++) { 			//Comeca a colocar dados no hashmap das tags
				for (int j = 0; j < intersect[i].size(); j++) { 
					if (tableTag_map.Procura_map(intersect[i][j]) == 0) {
						tableTag_map.Insercao_map(intersect[i][j]);
					}
				}
			}

			vector<int> vetor_de_ids;
			vetor_de_ids = tableTag_map.Procura_interseccao(tokens.size());		//Procura as interseccoes de codigo
			struct jogadores* valueIdTags;

			//Inicio da parte de printagem na tela dos tags
			printf("FIFA_ID			NOME		POS			RATING			COUNT\n\n");
			
			for (int i = 0; i < vetor_de_ids.size(); i++) { 

				valueIdTags = table.Procura(vetor_de_ids[i]);
				cout << " " << valueIdTags->id << " " << valueIdTags->name << " " << valueIdTags->position << " " << valueIdTags->rating << " " << valueIdTags->count << std::endl;

			}
		}


	}

	return 0;

}
