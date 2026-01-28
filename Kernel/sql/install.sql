-- sql/install.sql

\if :{?libpath}
\else
   \echo 'Error: [libpath] variable is not set.'
   \echo 'Usage: psql -v libpath="/abs/path/to/lib.so" -f install.sql'
   \quit
\endif

DROP FUNCTION IF EXISTS mqo_dispatch(bytea);
DROP FUNCTION IF EXISTS mqo_debug(bytea);


CREATE FUNCTION mqo_dispatch(bytea)
RETURNS integer
AS :'libpath', 'mqo_dispatch'
LANGUAGE C STRICT;

CREATE FUNCTION mqo_debug(bytea)
RETURNS text
AS :'libpath', 'mqo_debug'
LANGUAGE C STRICT;

\echo 'LumosKernel installation attempt finished.'
