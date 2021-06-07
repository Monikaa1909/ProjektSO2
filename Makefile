
CC = gcc

CFLAGS = -g -Wall -pthread

NAME1 = starveR

NAME2 = starveW

TARGET1 = zaglodzenieCzytelnikow.c

TARGET2 = zaglodzeniePisarzy.c

$(NAME1): $(TARGET1)
	$(CC) $(CFLAGS) -o $(NAME1) $(TARGET1)

$(NAME2): $(TARGET2)
	$(CC) $(CFLAGS) -o $(NAME2) $(TARGET2)

clean:
	$(RM) starve*
