#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int MAX_CUSTOMER_TIME;
int MAX_CASHIER_QUEUE;
int MAX_CASHIERS;
int MAX_NEXT_CUSTOMERS;

typedef struct Person {
	char _name;
	int _time;
} Person;

typedef struct QueueNode {
	Person *person;
	struct QueueNode *next;
} QueueNode;

typedef struct Queue {
	QueueNode *head;
	QueueNode *tail;
	int _size;
} Queue;

typedef struct Cashier {
	bool _work;
	int _served;
	Queue *queue;
} Cashier;

void loadConfig(FILE *configFile) {
	int config[4];	  // tmp array for settings value
	char setting[32]; // string for scan
	for (int i = 0; i < 4; ++i) {
		// split before "="
		char *key = strtok(fgets(setting, sizeof(setting), configFile), "=");
		// split before ";"
		// config[i] = atoi(strtok(NULL, ";"));
	}
	MAX_CUSTOMER_TIME = config[0];
	MAX_CASHIER_QUEUE = config[1];
	MAX_CASHIERS = config[2];
	MAX_NEXT_CUSTOMERS = config[3];
}

void loadFiles() {
	FILE *configFile = fopen("settings.txt", "r");
	if (configFile == NULL) {
		printf("Config file not found");
		exit(EXIT_FAILURE);
	}
	else
		loadConfig(configFile);
	FILE *gameoverFile = fopen("gameover.txt", "r");
	if (gameoverFile == NULL) {
		printf("GAMEOVER message file not found");
		exit(EXIT_FAILURE);
	}
	fclose(gameoverFile);
	fclose(configFile);
}

Person *newPerson() {
	Person *person = (Person *)malloc(sizeof(Person));
	person->_name = (char)((rand() % 26) + 97);
	person->_time = rand() % MAX_CUSTOMER_TIME + 1;
	return person;
}

char PersonGetName(Person *person) { return person->_name; }

int PersonGetTime(Person *person) { return person->_time; }

void PersonDecrementTime(Person *person) { person->_time--; }

QueueNode *newNode(Person *p) {
	QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
	node->person = p;
	node->next = NULL;
	return node;
}

Queue *newQueue() {
	Queue *queue = (Queue *)malloc(sizeof(Queue));
	queue->head = NULL;
	queue->tail = NULL;
	queue->_size = 0;
	return queue;
}

int QueueGetSize(Queue *queue) { return queue->_size; }

Cashier *newCashdesk() {
	Cashier *cashdesk = (Cashier *)malloc(sizeof(Cashier));
	cashdesk->queue = newQueue();
	cashdesk->_served = 0;
	cashdesk->_work = false;
	return cashdesk;
}

int CashdeskGetServed(Cashier *cashdesk) { return cashdesk->_served; }

void CashdeskIncrementServed(Cashier *cashdesk) { cashdesk->_served++; }

bool CashdeskGetWork(Cashier *cashdesk) { return cashdesk->_work; }

void CashdeskSetWork(Cashier *cashdesk, bool value) { cashdesk->_work = value; }

void QueuePush(Queue *q, QueueNode *node) {
	if (!node)
		node = newNode(newPerson());
	node->next = NULL;
	if (q->head) {
		q->tail->next = node;
		q->tail = node;
	} else
		q->head = q->tail = node;
	q->_size++;
}

QueueNode *QueuePeek(Queue *queue) { return queue->head; }

QueueNode *QueuePop(Queue *queue) {
	QueueNode *node = QueuePeek(queue);
	if (node) {
		queue->head = queue->head->next;
		queue->_size--;
		return node;
	} else
		return NULL;
}

void CashdeskQueuePop(Cashier *cashier) {
	QueueNode *node = QueuePop(cashier->queue);
	CashdeskIncrementServed(cashier);
	if (node) {
		free(node->person);
		free(node);
	}
}

bool CashdeskPush(Cashier **cashdesks, QueueNode *node) {
	Cashier *maxCustomersCashdesk = NULL;
	for (int i = 0; i < MAX_CASHIERS; ++i) {
		if (QueueGetSize(cashdesks[i]->queue) == MAX_CASHIER_QUEUE)
			continue;
		if (!maxCustomersCashdesk ||
			CashdeskGetServed(cashdesks[i]) >
				CashdeskGetServed(maxCustomersCashdesk)) {
			maxCustomersCashdesk = cashdesks[i];
		}
	}
	if (maxCustomersCashdesk) {
		CashdeskSetWork(maxCustomersCashdesk, true);
		QueuePush(maxCustomersCashdesk->queue, node);
	}
	return maxCustomersCashdesk == NULL;
}

bool CashdeskQueuePush(Cashier **cashdesks, Queue *nextCustomers) {
	bool GAMEOVER_FLAG = false;
	QueueNode *node = QueuePop(nextCustomers);
	while (node && !GAMEOVER_FLAG) {
		GAMEOVER_FLAG = CashdeskPush(cashdesks, node);
		node = QueuePop(nextCustomers);
	}
	return GAMEOVER_FLAG;
}

void CashdeskQueueDisplay(Cashier **cashdesks) {
	QueueNode **currentCustomers =
		(QueueNode **)malloc(MAX_CASHIERS * sizeof(QueueNode *));
	for (int i = 0; i < MAX_CASHIERS; ++i) {
		currentCustomers[i] = QueuePeek(cashdesks[i]->queue);
	}
	for (int i = 0; i < MAX_CASHIER_QUEUE; ++i) {
		for (int j = 0; j < MAX_CASHIERS; ++j) {
			if (currentCustomers[j]) {
				printf("\t%c%d", PersonGetName(currentCustomers[j]->person),
					   PersonGetTime(currentCustomers[j]->person));
				currentCustomers[j] = currentCustomers[j]->next;
			} else
				printf("\t||");
		}
		if (i < MAX_CASHIER_QUEUE - 1)
			printf("\n");
	}
	free(currentCustomers);
}

void CashdeskServe(Cashier *cashier) {
	Person *person;
	if (QueuePeek(cashier->queue))
		person = QueuePeek(cashier->queue)->person;
	else
		return;
	PersonDecrementTime(person);
	if (PersonGetTime(person) == 0)
		CashdeskQueuePop(cashier);
	if (QueueGetSize(cashier->queue) == 0)
		CashdeskSetWork(cashier, false);
}

void freePerson(Person *person) { free(person); }

void freeNode(QueueNode *node) {
	freePerson(node->person);
	free(node);
}

void freeQueue(Queue *queue) {
	int queueSize = QueueGetSize(queue);
	for (int i = 0; i < queueSize; ++i) {
		freeNode(QueuePop(queue));
	}
	free(queue);
}

void freeCashdesks(Cashier **cashdesks) {
	for (int i = 0; i < MAX_CASHIERS; ++i) {
		freeQueue(cashdesks[i]->queue);
		free(cashdesks[i]);
	}
	free(cashdesks);
}

void QueueNextCustomersDisplay(Queue *q) {
	printf("\nСледующие посетители: ");
	QueueNode *node = QueuePeek(q);
	while (node) {
		printf("%c%d ", PersonGetName(node->person),
			   PersonGetTime(node->person));
		node = node->next;
	}
}

int QueueCustomersCountDisplay(Cashier **cashdesks) {
	int value = 0;
	for (int i = 0; i < MAX_CASHIERS; ++i) {
		value += QueueGetSize(cashdesks[i]->queue);
	}
	return value;
}

int CashdesksServedCustomersCountDisplay(Cashier **cashdesks) {
	int value = 0;
	for (int i = 0; i < MAX_CASHIERS; ++i) {
		value += CashdeskGetServed(cashdesks[i]);
	}
	return value;
}

int CashdesksWorkCountDisplay(Cashier **cashdesks) {
	int value = 0;
	for (int i = 0; i < MAX_CASHIERS; ++i) {
		value += CashdeskGetWork(cashdesks[i]);
	}
	return value;
}

void clearConsole() {
#if __unix__ || __linux__ || __APPLE__ || __MACH__
	system("clear");
#elif _WIN32
	system("cls");
#else
	puts("what the OS says?");
#endif
}

void renderInterface(int globalTime, Queue *nextCustomers,
					 Cashier **cashdesks) {
	clearConsole();
	// display Remi
	printf("Супермаркет \"Реми\". Система моделирования очередей.\n");
	// display cashdesk number
	for (int i = 1; i <= MAX_CASHIERS; i++)
		printf("\t%d", i);
	printf("\n");
	// display cashdesk's _served customers
	for (int i = 0; i < MAX_CASHIERS; i++) {
		printf("\t%d", CashdeskGetServed(cashdesks[i]));
	}
	printf("\n");
	// display cashdesk mood (on/off)
	for (int i = 0; i < MAX_CASHIERS; i++) {
		printf("\t%c", CashdeskGetWork(cashdesks[i]) ? '+' : '-');
	}
	printf("\n");
	// display queues of cashdesks
	CashdeskQueueDisplay(cashdesks);
	printf("\nВремя: %d", globalTime);
	QueueNextCustomersDisplay(nextCustomers);
	printf("\nЧеловек в очередях: %d", QueueCustomersCountDisplay(cashdesks));
	printf("\nКасс работает: %d из %d", CashdesksWorkCountDisplay(cashdesks),
		   MAX_CASHIERS);
	printf("\nВсего обслужено: %d",
		   CashdesksServedCustomersCountDisplay(cashdesks));
	printf("\nДопустимая очередь на кассу: %d\n", MAX_CASHIER_QUEUE);
}

void GameoverDisplay() {
	clearConsole();
	FILE *gameoverFile = fopen("gameover.txt", "r");
	char message[255]; // string for scan
	while (feof(gameoverFile) == 0) {
		fgets(message, sizeof(message), gameoverFile);
		printf(message);
	}
	fclose(gameoverFile);
}

int main() {
	srand(time(NULL)); // randomize seed
	// load config
	loadFiles();

	long long globalTime = 0; // global timer
	Cashier **cashdesks = (Cashier **)malloc(
		MAX_CASHIERS * sizeof(Cashier *)); // array of cashdesks
	Queue *nextCustomers = newQueue();	   // next customers queue
	for (int i = 0; i < MAX_CASHIERS; i++) {
		cashdesks[i] = newCashdesk();
	}
	while (true) {
		// Push new customers to next customers queue
		for (int i = 0; i < rand() % MAX_NEXT_CUSTOMERS + 1; ++i) {
			QueuePush(nextCustomers, NULL);
		}
		// display stats of Remi
		renderInterface(globalTime, nextCustomers, cashdesks);
		// cashdesk serve
		for (int i = 0; i < MAX_CASHIERS; ++i) {
			CashdeskServe(cashdesks[i]);
		}
		// distribution to the cashdesk queue
		if (CashdeskQueuePush(cashdesks, nextCustomers)) {
			renderInterface(globalTime, nextCustomers, cashdesks);
			GameoverDisplay();
			break;
		}
		++globalTime;
		sleep(1);
	}
	freeQueue(nextCustomers);
	freeCashdesks(cashdesks);
	return 0;
}
