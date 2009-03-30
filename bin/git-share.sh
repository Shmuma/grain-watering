#!/bin/sh

git push shared
(cd /var/www/scm/plaun.git; git update-server-info)
