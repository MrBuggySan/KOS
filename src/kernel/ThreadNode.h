
// Class for tree node
class ThreadNode{
	friend class Scheduler;
	Thread *th;

	public:
		bool operator < (ThreadNode other) const {
			return th->getPriority() < other.th->getPriority();
		}
		bool operator == (ThreadNode other) const {
			return th->getPriority() == other.th->getPriority();
		}
		bool operator > (ThreadNode other) const {
			return th->getPriority() > other.th->getPriority();
		}

	//this is how we want to do it
	ThreadNode(Thread *t){
		th = t;
	}
};
