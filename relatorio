1. Objetivo da Aplicação
A aplicação cliente-servidor desenvolvida visa gerenciar um sistema de tópicos de mensagens utilizando FIFOs para a comunicação entre o servidor e os clientes. O servidor centraliza o gerenciamento dos tópicos, enquanto os clientes podem interagir com o sistema enviando mensagens, subscrevendo tópicos e realizando o login. O uso de FIFOs permite uma comunicação eficiente e simplificada entre o servidor e múltiplos clientes simultaneamente.

2. Arquitetura e Estruturas de Dados
Comunicação via FIFOs
A comunicação entre o servidor e os clientes é realizada por meio de FIFOs, permitindo uma interação síncrona e assíncrona entre o servidor e os clientes. Cada cliente possui um FIFO de feed próprio, nomeado como F_%d, onde %d representa o PID (Process ID) do cliente. O servidor, por sua vez, usa um FIFO principal de gerenciamento denominado M_PIPE.

Estruturas de Dados Utilizadas:
As estruturas de dados são projetadas para armazenar informações relevantes sobre os usuários, tópicos, mensagens e pedidos.
    • LOGIN: Armazena o PID do cliente e o nome de usuário.

	
	typedef struct {
    		int pid;
    		char username[TAM_NOME];
	} LOGIN;

RESPOSTA: Estrutura para armazenar o pid do cliente e uma lista de tópicos.
	typedef struct {
   		 int pid;
    		char str[TAM_TOPICO * MAX_TOPICS];
	} RESPOSTA;

MENSAGEM: Estrutura que representa uma mensagem enviada em um tópico. Contém o tópico, o corpo da mensagem, duração, e o pid do remetente.
	typedef struct {
    		char topico[TAM_TOPICO];
    		char corpo_msg[TAM_MSG];
    		int duracao;
    		int pid;
	}MENSAGEM;

TOPICO: Estrutura que representa um tópico, incluindo informações sobre bloqueio, mensagens associadas, e os clientes inscritos.

	typedef struct {
    		char nome_topico[TAM];
    		bool bloqueado;
    		MENSAGEM mensagens[MAX_MSG_PER];
    		int nMsgs;
    		int upid;
    		int subscritos_pid[MAX_USERS];
    		int nSubs;
	} TOPICO;

PEDIDO: Estrutura genérica para encapsular diferentes tipos de dados (login, mensagem, resposta e tópico).
	typedef struct {
    		int tipo;  // Tipo de estrutura
    		union {
        			LOGIN l;
        			MENSAGEM m;
        			RESPOSTA r;
        			TOPICO t;
    		};
	} PEDIDO;

DATA: Estrutura principal do servidor, contendo informações sobre os usuários conectados, tópicos criados, e variáveis de controle como o trincamento (mutex) e o estado de pedidos.

	typedef struct {
   		 int nUsers;
    		int users_pids[MAX_USERS];
    		char users_names[MAX_USERS][TAM_NOME];
    		TOPICO topicos[MAX_TOPICS];
    		int nTopicos;
    		bool pedidos_on;
    		pthread_mutex_t *ptrinco;
	} DATA;
.
3. Controles e Limitações
Para garantir o bom funcionamento do sistema, foram estabelecidos limites em várias áreas do sistema:
    • Número máximo de usuários: 10 usuários simultâneos.
    • Número máximo de tópicos: 20 tópicos.
    • Número máximo de mensagens por tópico: 5 mensagens.
    • Bloqueio de tópicos: Tópicos podem ser bloqueados, impedindo a adição de novas mensagens.
    • Exclusão mútua (Mutex): Para garantir a segurança no acesso a dados compartilhados entre múltiplos clientes, o servidor utiliza um mutex (ptrinco).
Essas limitações garantem a integridade do sistema e evitam sobrecarga de recursos.
Funções no Servidor
1. Função envia_info
    • Motivo:
        ◦ Necessidade de enviar respostas específicas para cada cliente sem interromper outros processos.
        ◦ Implementa comunicação unidirecional via FIFO específico do cliente (FIFO feed).
        ◦ Garante que a comunicação entre servidor e cliente seja personalizada, enviando dados diretamente para o PID do cliente requisitante.
2. Função recebe_pedidos
    • Motivo:
        ◦ Gerenciar de forma contínua os pedidos dos clientes, independentemente do tipo de pedido.
        ◦ A execução em uma thread separada permite o processamento assíncrono e simultâneo de múltiplos pedidos, melhorando a escalabilidade.
        ◦ O uso de um switch/case para lidar com diferentes tipos de pedidos mantém o código organizado e facilita futuras expansões.
3. Função mostra_users
    • Motivo:
        ◦ Fornece visibilidade ao administrador sobre os usuários conectados, seus PIDs e seus nomes de usuário.
        ◦ Facilita a administração, permitindo ações como identificação de usuários ativos e gestão de permissões, se necessário.
4. Função mostra_subs
    • Motivo:
        ◦ Exibe quais usuários estão inscritos em um tópico específico, o que é útil para análise e gerenciamento pelo administrador.
        ◦ Facilita o monitoramento de atividades dentro de tópicos específicos.
5. Função mostra_msgs
    • Motivo:
        ◦ Permite ao administrador visualizar as mensagens armazenadas em um tópico.
        ◦ Útil para auditoria ou resolução de problemas relacionados ao envio de mensagens, especialmente em cenários de mensagens persistentes ou expiradas.
6. Função mostra_topicos
    • Motivo:
        ◦ Exibe todos os tópicos disponíveis, incluindo seu estado (bloqueado ou desbloqueado), para o administrador.
        ◦ Essencial para gerenciar tópicos, verificando quais estão ativos ou quais precisam de manutenção.
7. Função bloqueia_topico
    • Motivo:
        ◦ Restringe o envio de novas mensagens para tópicos específicos, útil em situações de manutenção, spam ou uso indevido.
        ◦ O bloqueio evita que tópicos sejam sobrecarregados, protegendo a integridade do sistema.
8. Função desbloqueia_topico
    • Motivo:
        ◦ Reativa tópicos bloqueados para permitir novamente o envio de mensagens.
        ◦ Garante flexibilidade na gestão do servidor, permitindo alternar o estado dos tópicos conforme necessário.
9. Função main
    • Motivo:
        ◦ Centraliza a inicialização do servidor e o gerenciamento das threads que lidam com pedidos.
        ◦ Inclui uma interface para o administrador, permitindo que ele execute comandos e gerencie o sistema de forma interativa.
        ◦ Sua estrutura modular facilita a inclusão de novas funcionalidades sem comprometer a organização do código.

Funções no Cliente
1. Função timeout
    • Motivo:
        ◦ Implementa um mecanismo de controlo para detectar inatividade ou problemas de comunicação com o servidor.
        ◦ Melhora a experiência do usuário, fornecendo feedback imediato em casos de falha na resposta do servidor.
2. Função login
    • Motivo:
        ◦ Identifica e autentica cada cliente no servidor, associando seu PID a um nome de usuário.
        ◦ Essencial para controlar o acesso ao sistema e registrar usuários de forma unívoca, permitindo funcionalidades como associação de mensagens e inscrições a usuários específicos.

3. Função envia_pedido
    • Motivo:
        ◦ Centraliza o envio de pedidos ao servidor, reduzindo redundância no código do cliente.
        ◦ Simplifica a lógica para diferentes tipos de comandos, tornando o cliente mais intuitivo e modular.
        ◦ Abstrai a complexidade de construção de pacotes, facilitando a interação do usuário com o sistema.
4. Função main
    • Motivo:
        ◦ Inicializa o cliente e gerencia a interface principal, permitindo que o usuário interaja com o servidor de maneira simples e eficiente.
        ◦ Agrupa as operações de login, envio de pedidos e tratamento de respostas, garantindo uma experiência contínua.

Razões Gerais para as Escolhas
    1. Separação de Responsabilidades:
        ◦ As funções no cliente focam na interação com o sistema, enquanto as funções no servidor lidam com a lógica de gerenciamento.
        ◦ Essa separação melhora a organização e modularidade do código.
    2. Escalabilidade:
        ◦ A thread para recebe_pedidos no servidor garante que novos pedidos sejam processados sem impactar os pedidos em andamento.
        ◦ FIFOs específicos para cada cliente permitem múltiplas conexões simultâneas.
    3. Gerenciamento e Controlo:
        ◦ Funções como mostra_users e mostra_topicos fornecem ao administrador ferramentas para monitorar e ajustar o sistema em tempo real.
        ◦ Os controles de bloqueio e desbloqueio de tópicos ajudam a evitar abusos e a gerenciar melhor os recursos.
    4. Facilidade de Uso e Expansão:
        ◦ A estrutura do sistema foi projetada para ser expansível. Por exemplo, novas funções de cliente ou tipos de pedido podem ser adicionados facilmente ao switch/case do servidor.
        ◦ O uso de mutexes para garantir a consistência dos dados compartilhados protege contra condições de corrida e falhas relacionadas.

5. Conclusão

	A aplicação cliente-servidor baseada em FIFOs proporciona uma plataforma robusta e eficiente para o gerenciamento de tópicos de mensagens. A comunicação via FIFOs é simples, porém eficaz, permitindo que o servidor gerencie múltiplos clientes e tópicos simultaneamente.
O uso de estruturas de dados bem definidas e a implementação de mutexes garantem a integridade dos dados e a segurança no acesso a recursos compartilhados. As funções no servidor permitem gerenciar a interação dos usuários com os tópicos, incluindo a criação, envio de mensagens, inscrição e bloqueio de tópicos.
	Com esse sistema, os clientes têm uma interface simples para interação com o servidor, podendo se registrar, enviar mensagens para tópicos, e gerenciar suas inscrições de forma dinâmica. A aplicação, portanto, atende bem à proposta de gerenciar um sistema de tópicos de mensagens, mantendo uma comunicação eficiente e segura entre os clientes e o servidor.
