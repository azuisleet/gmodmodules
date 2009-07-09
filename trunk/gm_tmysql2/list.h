template<class T> 
class SyncList
{
	struct node {
		T value;
		node *next;	
	};
	node *head;
	node *tail;
	int size;
	Mutex *m_lock;
public:
	SyncList(): size(0), head(NULL), tail(NULL) { m_lock = new Mutex; };
	~SyncList() { delete m_lock; }
	int getSize()
	{
		int sz;
		m_lock->Acquire();
		sz = size;
		m_lock->Release();
		return sz;
	}

	void add(T data)
	{
		node *b = new node;
		b->next = NULL;
		b->value = data;

		m_lock->Acquire();
		if(head == NULL)
		{
			head = b;
			tail = b;
		} else {
			tail->next = b;
			tail = b;
		}
		size++;
		m_lock->Release();
	}

	T remove()
	{
		T result;
		node *tmp;

		m_lock->Acquire();
		if(head == NULL)
		{
			m_lock->Release();
			return NULL;
		}
		tmp = head;
		head = tmp->next;
		size--;
		if(size == 0) head = NULL;
		m_lock->Release();

		result = tmp->value;
		delete tmp;
		return result;
	}
};