struct job
{

    unsigned int id;
    unsigned int ninputs;
    unsigned int ndatas;
    void *data[32];

};

void job_interpret(struct job *jobs, unsigned int njobs, struct event_header *iheader, struct event_header *oheader, void *buffer, unsigned int count, unsigned int session);
