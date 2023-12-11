#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_CLIENTS 5

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;
int client_count = 0;

void* client_thread(void* arg) {
  int client_id = *((int*)arg);

  pthread_mutex_lock(&mutex);
  client_count--;
  pthread_cond_signal(&cond_var);
  pthread_mutex_unlock(&mutex);

  pthread_exit(NULL);
}

// 서버 스레드 함수
void* server_thread(void* arg) {
  while (1) {
    // 뮤텍스를 이용하여 모든 클라이언트 스레드가 작업을 마칠 때까지 대기
    pthread_mutex_lock(&mutex);
    while (client_count > 0) {
      pthread_cond_wait(&cond_var, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    // 클라이언트에게 메시지 발송
    printf("Broadcasting message to all clients\n");

    // 뮤텍스를 이용하여 클라이언트 쓰레드가 작업을 시작할 수 있도록 신호 전송
    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond_var);
    pthread_mutex_unlock(&mutex);

    sleep(1);
  }
}

int main() {
  pthread_t server_tid, client_tid[MAX_CLIENTS];
  int client_id[MAX_CLIENTS];

  // 서버 스레드 생성
  pthread_create(&server_tid, NULL, server_thread, NULL);

  // 클라이언트 스레드 생성
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    client_id[i] = i;
    pthread_create(&client_tid[i], NULL, client_thread, (void*)&client_id[i]);
    pthread_mutex_lock(&mutex);
    client_count++;
    pthread_mutex_unlock(&mutex);
  }

  // 스레드의 종료를 기다리기
  pthread_join(server_tid, NULL);
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    pthread_join(client_tid[i], NULL);
  }

  return 0;
}