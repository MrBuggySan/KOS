
// Class for tree node
class ThreadNode{
	friend class Scheduler;
	Thread *th;

	public:
		bool operator < (ThreadNode other) const {
			// return th->getPriority() < other.th->getPriority();

        //We must compare based on vRuntime. 
      return th->getVRuntime() < other.th->getVRuntime();
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
