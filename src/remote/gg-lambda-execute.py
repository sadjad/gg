#!/usr/bin/env python3.6

import os
import sys
import fcntl
import logging
import json
import pprint
import boto3
import aiohttp
import asyncio

import s3
import gg
import gg_pb2

from contextlib import closing

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

CHUNK_SIZE = 32 * 1024 # 32 KB

s3_bucket = 'gg-us-west-2'
s3_client = boto3.client('s3')
lambda_client = boto3.client('lambda')

aws_credentials = boto3.Session().get_credentials()
auth_generator = s3.S3AuthGeneator(aws_credentials.access_key, aws_credentials.secret_key)

@aiohttp.streamer
def file_sender(writer, file_name=None):
    with open(file_name, 'rb') as f:
        chunk = f.read(CHUNK_SIZE)
        while chunk:
            yield from writer.write(chunk)
            chunk = f.read(CHUNK_SIZE)

async def upload_dependency(session: aiohttp.ClientSession, dep_hash: str):
    filename = gg.blob_path(dep_hash)

    object_url = s3.get_object_url(s3_bucket, dep_hash)
    headers = {}
    auth_generator('HEAD', s3_bucket, dep_hash, headers)

    async with session.head(object_url, headers=headers) as r:
        if r.status == 200:
            logger.info('Blob already exists: ' + dep_hash)
            return

    headers = {
        'x-amz-acl': 'public-read',
        'x-amz-tagging': 'gg=file',
        'Content-Length': '%d' % os.path.getsize(filename)
    }

    auth_generator('PUT', s3_bucket, dep_hash, headers)

    async with session.request('PUT',object_url, headers=headers, data=file_sender(filename)) as r:
        assert r.status == 200
        logger.info('Upload done: ' + dep_hash)

@asyncio.coroutine
def upload_all_dependencies(dependecies):
    with aiohttp.ClientSession() as session:
        upload_futures = [upload_dependency(session, d['hash']) for d in dependecies]

        for upload_future in asyncio.as_completed(upload_futures):
            result = yield from upload_future

def push_infiles(infiles):
    with closing(asyncio.get_event_loop()) as loop:
       loop.run_until_complete(upload_all_dependencies(infiles))

def main():
    thunk_hash = sys.argv[1]
    thunk = gg.read_thunk(thunk_hash)

    infiles = [{'hash': i.hash, 'executable': i.type == gg_pb2.InFile.Type.Value('EXECUTABLE')}
               for i in thunk.infiles
               if i.type != gg_pb2.InFile.Type.Value('DUMMY_DIRECTORY')]

    infiles += [{'hash': thunk_hash, 'executable': False}]

    payload = {
        'thunk_hash': thunk_hash,
        's3_bucket': s3_bucket,
        'infiles': infiles
    }

    push_infiles(infiles)

    response = lambda_client.invoke(
        FunctionName='ggfunction',
        InvocationType='RequestResponse',
        Payload=json.dumps(payload)
    )

    response = json.loads(response['Payload'].read())
    output_hash = response['output_hash']
    s3_client.download_file(s3_bucket, response['output_hash'], gg.blob_path(output_hash))

    print(output_hash)

if __name__ == '__main__':
    main()

# with closing(asyncio.get_event_loop()) as loop:
#    with aiohttp.ClientSession() as session:
#        loop.run_until_complete(download_multiple(session, []))