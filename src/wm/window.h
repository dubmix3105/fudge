struct window
{

    struct list_item item;
    unsigned int active;
    struct box size;
    unsigned int source;

};

void window_draw(struct window *window, unsigned int line);
void window_init(struct window *window);