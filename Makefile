
CC = gcc

CFLAGS = -g -Wall -pthread

NAME1 = starveR

NAME2 = starveW

NAME3 = starveNo

TARGET1 = zaglodzenieCzytelnikow.c

TARGET2 = zaglodzeniePisarzy.c

TARGET3 = zaglodzeNIE.c

$(NAME1): $(TARGET1)
	$(CC) $(CFLAGS) -o $(NAME1) $(TARGET1)

$(NAME2): $(TARGET2)
	$(CC) $(CFLAGS) -o $(NAME2) $(TARGET2)

$(NAME3): $(TARGET3)
	$(CC) $(CFLAGS) -o $(NAME3) $(TARGET3)

clean:
	$(RM) starve*
