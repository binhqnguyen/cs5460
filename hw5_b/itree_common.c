/* Generic part */

typedef struct {
	block_t	*p;//pointer to first level block
	block_t	key;//pointer to real block. 0 IF NOT IN BUFFER (init val = 0)
	struct buffer_head *bh;
} Indirect;

typedef struct{
	block_t bg;
	int len;
	int offset; //specify which extent this is in the inode.
} Extent; //An extent in the inode: <block value, len>

static DEFINE_RWLOCK(pointers_lock);

static int is_valid_block(struct inode *inode, long block){
	char b[BDEVNAME_SIZE];
	struct super_block *sb = inode->i_sb;
	int retval = 1;

	if (block < 0) {
		printk("MINIX-fs: block_to_path: block %ld < 0 on dev %s\n",block, bdevname(sb->s_bdev, b));
		retval = 0;
		
	} else 
	//if (block >= (minix_sb(inode->i_sb)->s_max_size/sb->s_blocksize)) {
	if (block > NBLOCKS_INODE_MAX){	
		if (printk_ratelimit())
			printk(KERN_EMERG "MINIX-fs: block_to_path: "
			       "block %ld too big on dev %s\n",
				block, bdevname(sb->s_bdev, b));
		retval = 0;
	}
	return retval;
}

static inline void add_chain(Indirect *p, struct buffer_head *bh, block_t *v)
{
	p->key = *(p->p = v);
	p->bh = bh;
}

static inline int verify_chain(Indirect *from, Indirect *to)
{
	while (from <= to && from->key == *from->p)
		from++;
	return (from > to);
}

static inline block_t *block_end(struct buffer_head *bh)
{
	return (block_t *)((char*)bh->b_data + bh->b_size);
}

static inline Indirect *get_branch(struct inode *inode,
					int depth,
					int *offsets,
					Indirect chain[DEPTH],
					int *err)
{
	struct super_block *sb = inode->i_sb;
	Indirect *p = chain;
	struct buffer_head *bh;

	*err = 0;
	/* i_data is not going away, no lock needed */
	add_chain (chain, NULL, i_data(inode) + *offsets);
	if (!p->key)
		goto no_block;
	while (--depth) {
		bh = sb_bread(sb, block_to_cpu(p->key));
		if (!bh)
			goto failure;
		read_lock(&pointers_lock);
		if (!verify_chain(chain, p))
			goto changed;
		add_chain(++p, bh, (block_t *)bh->b_data + *++offsets);
		read_unlock(&pointers_lock);
		if (!p->key)
			goto no_block;
	}
	return NULL;

changed:
	read_unlock(&pointers_lock);
	brelse(bh);
	*err = -EAGAIN;
	goto no_block;
failure:
	*err = -EIO;
no_block:
	return p;
}

static int alloc_branch(struct inode *inode,
			     int num,
			     int *offsets,
			     Indirect *branch)
{
	int n = 0;
	int i;
	int parent = minix_new_block(inode);

	branch[0].key = cpu_to_block(parent);
	if (parent) for (n = 1; n < num; n++) {
		struct buffer_head *bh;
		/* Allocate the next block */
		int nr = minix_new_block(inode);
		if (!nr)
			break;
		branch[n].key = cpu_to_block(nr);
		bh = sb_getblk(inode->i_sb, parent);
		lock_buffer(bh);
		memset(bh->b_data, 0, bh->b_size);
		branch[n].bh = bh;
		branch[n].p = (block_t*) bh->b_data + offsets[n];
		*branch[n].p = branch[n].key;
		set_buffer_uptodate(bh);
		unlock_buffer(bh);
		mark_buffer_dirty_inode(bh, inode);
		parent = nr;
	}
	if (n == num)
		return 0;

	/* Allocation failed, free what we already allocated */
	for (i = 1; i < n; i++)
		bforget(branch[i].bh);
	for (i = 0; i < n; i++)
		minix_free_block(inode, block_to_cpu(branch[i].key));
	return -ENOSPC;
}

static inline int splice_branch(struct inode *inode,
				     Indirect chain[DEPTH],
				     Indirect *where,
				     int num)
{
	int i;

	write_lock(&pointers_lock);

	/* Verify that place we are splicing to is still there and vacant */
	if (!verify_chain(chain, where-1) || *where->p)
		goto changed;

	*where->p = where->key;

	write_unlock(&pointers_lock);

	/* We are done with atomic stuff, now do the rest of housekeeping */

	inode->i_ctime = CURRENT_TIME_SEC;

	/* had we spliced it onto indirect block? */
	if (where->bh)
		mark_buffer_dirty_inode(where->bh, inode);

	mark_inode_dirty(inode);
	return 0;

changed:
	write_unlock(&pointers_lock);
	for (i = 1; i < num; i++)
		bforget(where[i].bh);
	for (i = 0; i < num; i++)
		minix_free_block(inode, block_to_cpu(where[i].key));
	return -EAGAIN;
}

static void print_inode(struct inode *inode){
	int i  = 0 ;
	printk(KERN_EMERG "inode: ");
	for (i=0;i<DIRECT;++i){
		printk(KERN_EMERG "%d\t", (int)*(i_data(inode)+i));
	}
	printk(KERN_EMERG "\n");
}

static Extent get_last_extent(struct inode *inode){
	block_t bl_value = 0;
	block_t bg = 0;
	int len = 0, sec = 0;
	Extent retval;
	retval.bg = 0;
	retval.len = 0;
	retval.offset = -1;

	print_inode(inode);
	while ( sec < DIRECT) { //check all non empty sector in inode.
		bl_value = *(i_data(inode)+sec);
		bg = bl_value >> LEN;
		len = (bl_value << BLOCK_BITS ) >> BLOCK_BITS;	//length.
		if (bg == 0){ //hit the first empty extent	
			printk(KERN_EMERG "get_last_extent: hit! offset = %d\n", sec-1);
			if (sec==0) goto ret;	//if inode is empty
			bl_value = *(i_data(inode)+sec-1);
			bg = bl_value >> LEN;
			len = (bl_value << BLOCK_BITS ) >> BLOCK_BITS;	//length.
			retval.bg = bg;	
			retval.len = len;
			retval.offset = sec-1;
			printk(KERN_EMERG "xxxx last_extent <bg,len> =  <%d,%d>\n",(int)bg, (int)len);
			goto ret;
		} 
		sec++; //next sector
	}
ret:
	return retval;	

}

static int update_inode(struct inode *inode, Extent last_extent){
	int retval = 0;
	//int bg = 0, len = 0;
	block_t block_val = 0;
	block_val = ((int)last_extent.bg << LEN) | last_extent.len;
	printk(KERN_EMERG "update_inode: bg = %d, len = %d, block_val = %d, offset = %d\n", (int)last_extent.bg, (int)last_extent.len, (int)block_val, (int) last_extent.offset);
	minix_i(inode)->u.i2_data[last_extent.offset+1] = (int)block_val;
	mark_inode_dirty(inode);
	print_inode(inode);
	return retval;
}

static int alloc_extent(struct inode *inode, int *block_bg){
	Extent last_extent;
	*block_bg = minix_new_block(inode);
	if (*block_bg!=0){ //succeed allocated the block
		//Check if this new block is in the contiguous region.
		//If not, add new extent on inode.
		last_extent = get_last_extent(inode);
		printk(KERN_EMERG "alloc_extent: new block = %d, last_extent.bg = %d, last_extent.len = %d\n",*block_bg,(int)last_extent.bg,(int)last_extent.len);
		if ( *block_bg == last_extent.bg+last_extent.len ){//if contiguous
			printk(KERN_EMERG "alloc_extent: contiguous\n");
			last_extent.len++;
		}
		else{	//not contiguous
			printk(KERN_EMERG "alloc_extent: not contiguous, new extent <%d,%d>\n",*block_bg,0);
			last_extent.bg = *block_bg;
			last_extent.len = 0;
		}
		update_inode(inode, last_extent);
		return 0;
	}
	return -ENOSPC;
}

/*obtain block value of the specified sector (of inode)*/
/*Return 0 if not found*/
static block_t get_block_value(struct inode *inode, sector_t block){
	uint32_t bl_value = 0;
	uint32_t bg = 0, len = 0, sum_len = 0, sec = 0, retval = 0;
	while ( (sec < DIRECT) ){ //check all non empty sector in inode.
		bl_value = *(i_data(inode)+sec);
		bg = bl_value >> LEN;
		len = (bl_value << BLOCK_BITS ) >> BLOCK_BITS;	//length.
		printk(KERN_EMERG "**get_block_value: reading <bg,len> = <%d,%d>\n",(int)bg,(int)len);
		sum_len += len;
		if (sum_len > block){
			sum_len -= len; //last sector.
			retval = (int) (bg + block - sum_len);
			printk(KERN_EMERG "**get_block_value: FOUND sector = %d; extent = %d; bg = %d; offset = %d; value = %d\n",(int)block,(int)sec,(int)bg,(int)(block-sum_len), retval);
			return retval; //bg = begin block of the extent. (block-sum_len)=offset of the block in the CONTIGUOUS region. 
		} 
		sec++; //next sector
	}
not_found:
	return 0;
}
/*load a block from disk to buffer_head bh*/
static inline int get_block(struct inode * inode, sector_t block,
			struct buffer_head *bh, int create)
{
	int err = -EIO;
	//unsigned long block_value = 0;
	int block_bg = 0;
	//int length = 0;

	printk(KERN_INFO "itree_common: get_block,sector = %d\n",(int)block);
	if (!is_valid_block(inode, block))/*check for valid block*/
		goto out;
	block_bg = get_block_value(inode, block);
	//block_value = *(i_data(inode)+block); //get the value contain in the zone inside the inode.
	//block_bg = block_value >> LEN;
	//length = (block_value << BLOCK_BITS) >> BLOCK_BITS;
reread:
	//printk(KERN_EMERG "get_block: reading sector = %d, block_value = %d\n", (int)block, (int)block_bg);
	if (block_bg!=0){	//block!=0, found on disk
		map_bh(bh, inode->i_sb, block_bg);	

		printk(KERN_INFO "itree_common->get_block: block in sector %d found on disk, value= %d\n",(int)block,block_bg);
		err = 0; 	//succeed.
		goto out;
	}
	/* Next simple case - plain lookup or failed read of indirect block */
	if (!create) {
		printk(KERN_INFO "itree_common->get_block: can't find sector %d on inode\n",(int)block);
out:
		printk(KERN_EMERG "get_block returns err = %d\n",err);
		return err;
	}
	/*Block not found and creating needed, create a new block*/
	/*So far, block contains only block number, in the extent version, block contains both block number and length of region*/
	err = alloc_extent(inode, &block_bg);	
	if (err==0)	//succeed, reread block again
		goto reread;
	else{	//failed allocating a new block
		printk(KERN_INFO "itree_common->get_block: failed allocating new block\n");
		minix_free_block(inode,block);
		err = -ENOSPC;
		goto out;
	}
}

static inline int all_zeroes(block_t *p, block_t *q)
{
	while (p < q)
		if (*p++)
			return 0;
	return 1;
}

static Indirect *find_shared(struct inode *inode,
				int depth,
				int offsets[DEPTH],
				Indirect chain[DEPTH],
				block_t *top)
{
	Indirect *partial, *p;
	int k, err;

	*top = 0;
	for (k = depth; k > 1 && !offsets[k-1]; k--)
		;
	partial = get_branch(inode, k, offsets, chain, &err);

	write_lock(&pointers_lock);
	if (!partial)
		partial = chain + k-1;
	if (!partial->key && *partial->p) {
		write_unlock(&pointers_lock);
		goto no_top;
	}
	for (p=partial;p>chain && all_zeroes((block_t*)p->bh->b_data,p->p);p--)
		;
	if (p == chain + k - 1 && p > chain) {
		p->p--;
	} else {
		*top = *p->p;
		*p->p = 0;
	}
	write_unlock(&pointers_lock);

	while(partial > p)
	{
		brelse(partial->bh);
		partial--;
	}
no_top:
	return partial;
}

static inline void free_data(struct inode *inode, block_t *p, block_t *q)
{
	unsigned long nr;

	for ( ; p < q ; p++) {
		nr = block_to_cpu(*p);
		if (nr) {
			*p = 0;
			minix_free_block(inode, nr);
		}
	}
}

static void free_branches(struct inode *inode, block_t *p, block_t *q, int depth)
{
	struct buffer_head * bh;
	unsigned long nr;

	if (depth--) {
		for ( ; p < q ; p++) {
			nr = block_to_cpu(*p);
			if (!nr)
				continue;
			*p = 0;
			bh = sb_bread(inode->i_sb, nr);
			if (!bh)
				continue;
			free_branches(inode, (block_t*)bh->b_data,
				      block_end(bh), depth);
			bforget(bh);
			minix_free_block(inode, nr);
			mark_inode_dirty(inode);
		}
	} else
		free_data(inode, p, q);
}

static inline void truncate (struct inode * inode)
{
	struct super_block *sb = inode->i_sb;
	block_t *idata = i_data(inode);
	int offsets[DEPTH];
	Indirect chain[DEPTH];
	Indirect *partial;
	block_t nr = 0;
	int n;
	int first_whole;
	long iblock;
	
	printk(KERN_EMERG "itree_common: truncate\n");
	iblock = (inode->i_size + sb->s_blocksize -1) >> sb->s_blocksize_bits;
	block_truncate_page(inode->i_mapping, inode->i_size, get_block);

	n = block_to_path(inode, iblock, offsets);
	if (!n)
		return;

	if (n == 1) {
		free_data(inode, idata+offsets[0], idata + DIRECT);
		first_whole = 0;
		goto do_indirects;
	}

	first_whole = offsets[0] + 1 - DIRECT;
	partial = find_shared(inode, n, offsets, chain, &nr);
	if (nr) {
		if (partial == chain)
			mark_inode_dirty(inode);
		else
			mark_buffer_dirty_inode(partial->bh, inode);
		free_branches(inode, &nr, &nr+1, (chain+n-1) - partial);
	}
	/* Clear the ends of indirect blocks on the shared branch */
	while (partial > chain) {
		free_branches(inode, partial->p + 1, block_end(partial->bh),
				(chain+n-1) - partial);
		mark_buffer_dirty_inode(partial->bh, inode);
		brelse (partial->bh);
		partial--;
	}
do_indirects:
	/* Kill the remaining (whole) subtrees */
	while (first_whole < DEPTH-1) {
		nr = idata[DIRECT+first_whole];
		if (nr) {
			idata[DIRECT+first_whole] = 0;
			mark_inode_dirty(inode);
			free_branches(inode, &nr, &nr+1, first_whole+1);
		}
		first_whole++;
	}
	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	mark_inode_dirty(inode);
}

static inline unsigned nblocks(loff_t size, struct super_block *sb)
{
	int k = sb->s_blocksize_bits - 10;	//s_blocksize_bits=10.
	unsigned blocks, res;
	blocks = (size + sb->s_blocksize - 1) >> (BLOCK_SIZE_BITS + k);	//k=-4, BLOCK_SIZE_BITS=10 (1024B block).
	res = blocks;
	//if (blocks > direct) {
	//	printk(KERN_EMERG "nblocks: file have some extent\n");
	//}
	printk(KERN_EMERG "nblocks: size = %d, return %d\n", (int)size, (int)res);
	return res;
}

