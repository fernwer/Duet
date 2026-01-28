DROP FUNCTION IF EXISTS mqo_dispatch(bytea);
DROP FUNCTION IF EXISTS mqo_debug(bytea);

DO $$
BEGIN
    RAISE NOTICE 'LumosKernel functions removed.';
END $$;