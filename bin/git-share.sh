#!/bin/sh

git push shared
(cd /var/www/1/plaun.git; git update-server-info)