class Mutex
{
public:
	volatile long m_handle;
	inline Mutex() : m_handle(0) {};
	inline ~Mutex() {};

	inline void Acquire()
	{
		DWORD controller;
		DWORD thread = GetCurrentThreadId();
		if(m_handle == thread)
			return;
		for(;;)
		{
			controller = InterlockedCompareExchange(&m_handle, thread, 0);
			if(controller == 0)
				break;
			__asm pause;
			Sleep(0);
		}
	}

	inline bool AttemptAcquire()
	{
		DWORD thread = GetCurrentThreadId();
		if(m_handle == thread)
			return true;

		DWORD controller = InterlockedCompareExchange(&m_handle, thread, 0);
		if(controller == 0)
			return true;

		return false;
	}

	inline void Release()
	{
		InterlockedExchange(&m_handle, 0);
	}
};