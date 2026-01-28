-- sql/install.sql

-- 检查变量是否存在，不存在则提示报错退出
\if :{?libpath}
\else
   \echo 'Error: [libpath] variable is not set.'
   \echo 'Usage: psql -v libpath="/abs/path/to/lib.so" -f install.sql'
   \quit
\endif

-- 清理旧函数
DROP FUNCTION IF EXISTS mqo_dispatch(bytea);
DROP FUNCTION IF EXISTS mqo_debug(bytea);

-- 注册函数
-- 注意：这里使用 :'libpath'，psql 会自动将其替换为 '/path/to/...'
CREATE FUNCTION mqo_dispatch(bytea)
RETURNS integer
AS :'libpath', 'mqo_dispatch'
LANGUAGE C STRICT;

CREATE FUNCTION mqo_debug(bytea)
RETURNS text
AS :'libpath', 'mqo_debug'
LANGUAGE C STRICT;

\echo 'LumosKernel installation attempt finished.'

-- psql -d postgres \
--     -v libpath="$PWD/build/lumos_kernel.so" \
--     -f sql/install.sql