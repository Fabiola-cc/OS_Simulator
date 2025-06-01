struct Mutex {
    bool locked;
    QString ownerPid; // quién tiene el lock
    Mutex() : locked(false), ownerPid("") {}
};