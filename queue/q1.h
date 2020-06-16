#define ElementType int //存储数据元素的类型
//#define MAXSIZE 6 //存储数据元素的最大个数
#define ERROR -99 //ElementType的特殊值，标志错误
 
typedef struct {
    ElementType *data;
    int maxsize;
    int front; //记录队列头元素位置
    int rear; //记录队列尾元素位置
    int size; //存储数据元素的个数
}Queue;
 
Queue* CreateQueue(int sz) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (!q) {
        printf("Queue run out of space.\n");
        return NULL;
    }
    q->data = (ElementType*)malloc(sz * sizeof(ElementType));
    q->maxsize = sz;
    q->front = -1;
    q->rear = -1;
    q->size = 0;
    return q;
}
 
int IsFullQ(Queue* q) {
    return (q->size == q->maxsize);
}
 
void AddQ(Queue* q, ElementType item) {
    if (IsFullQ(q)) {
        printf("Queue is full.\n");
        return;
    }
    q->rear++;
    q->rear %= q->maxsize;
    q->size++;
    q->data[q->rear] = item;
}
 
int IsEmptyQ(Queue* q) {
    return (q->size == 0);
}
 
ElementType DeleteQ(Queue* q) {
    if (IsEmptyQ(q)) {
        printf("Empty queue.\n");
        return ERROR;
    }
    q->front++;
    q->front %= q->maxsize; //0 1 2 3 4 5
    q->size--;
    return q->data[q->front];
}
 
void PrintQueue(Queue* q) {
    if (IsEmptyQ(q)) {
        printf("Empty queue.\n");
        return;
    }
    printf("Print elements in queue:\n");
    int index = q->front;
    int i;
    for (i = 0; i < q->size; i++) {
        index++;
        index %= q->maxsize;
        printf("%d ", q->data[index]);
    }
    printf("\n");
}