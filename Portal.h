class Portal
{
private:
    bool lsOpened;
    bool lsClosed;
    unsigned char pinOutput;
public:
    Portal(unsigned char pinOutput, unsigned char pinLsOpened, unsigned char pinLsClosed);

    void press();
    void release();
    bool getLsOpened();
    bool getLsClosed();
};