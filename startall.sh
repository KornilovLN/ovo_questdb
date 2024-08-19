#!/bin/bash

# Запуск tmux сессии
tmux new-session -d -s ovo_session

# Окно 1: Запуск сборщика данных и отправки в ПК и далее - в БД
tmux send-keys -t ovo_session 'cd /mnt/poligon/ovo_questdb/ovo' C-m
tmux send-keys -t ovo_session 'echo "Запуск сборщика данных и отправки в ПК и далее - в БД"' C-m
tmux send-keys -t ovo_session './termcomm' C-m

# Окно 2: Запуск локального сервера просмотра данных из БД
tmux new-window -t ovo_session
tmux send-keys -t ovo_session 'cd /mnt/poligon/ovo_questdb/' C-m
tmux send-keys -t ovo_session 'echo "Запуск локального сервера просмотра данных из БД"' C-m
tmux send-keys -t ovo_session './app.py' C-m

# Окно 3: Чтение из БД в текстовом режиме данных сенсоров из БД
tmux new-window -t ovo_session
tmux send-keys -t ovo_session 'cd /mnt/poligon/ovo_questdb/' C-m
tmux send-keys -t ovo_session 'echo "Чтение из БД в текстовом режиме данных сенсоров из БД"' C-m
tmux send-keys -t ovo_session './reader_sensors.py' C-m

# Переключение на третье окно
tmux select-window -t ovo_session:2

# Attach to the tmux session
tmux attach -t ovo_session

